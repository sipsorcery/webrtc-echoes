/******************************************************************************
* Filename: PcFactory.cpp
*
* Description: See header file.
*
* Author:
* Aaron Clauson (aaron@sipsorcery.com)
*
* History:
* 08 Mar 2021	Aaron Clauson	  Created, Dublin, Ireland.
* 21 Dec 2024 Aaron Clauson   Updated for libwebrtc version m132.
*
* License: Public Domain (no warranty, use at own risk)
/******************************************************************************/

#include "PcFactory.h"
#include "json.hpp"

#include "api/audio/audio_processing.h"
#include "api/audio/builtin_audio_processing_builder.h"
#include "api/environment/environment.h"
#include "api/environment/environment_factory.h"
#include "api/audio_codecs/audio_decoder_factory_template.h"
#include "api/audio_codecs/audio_encoder_factory_template.h"
#include <api/audio_codecs/audio_decoder_factory.h>
#include <api/audio_codecs/audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include "api/audio_codecs/g711/audio_decoder_g711.h"
#include "api/audio_codecs/g711/audio_encoder_g711.h"
#include <api/create_peerconnection_factory.h>
#include <api/peer_connection_interface.h>
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/video_decoder_factory.h>
#include <api/video_codecs/video_encoder_factory.h>
#include <media/engine/webrtc_media_engine.h>
#include "api/enable_media.h"
#include "fake_audio_capture_module.h"

#include <iostream>
#include <sstream>

// Number of seconds to wait for the remote SDP offer to be set on the peer connection.
#define SET_REMOTE_SDP_TIMEOUT_SECONDS 3

PcFactory::PcFactory() :
  _peerConnections()
{
  std::cout << "PcFactory initialise on " << std::this_thread::get_id() << std::endl;

  SignalingThread = rtc::Thread::Create();
  SignalingThread->Start();

  webrtc::AudioProcessing::Config apmConfig;
  apmConfig.gain_controller1.enabled = false;
  apmConfig.gain_controller2.enabled = false;
  auto apm = webrtc::BuiltinAudioProcessingBuilder(apmConfig).Build(webrtc::CreateEnvironment());

   webrtc::PeerConnectionFactoryDependencies _pcf_deps;
  _pcf_deps.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
  _pcf_deps.signaling_thread = SignalingThread.get();
  _pcf_deps.event_log_factory = std::make_unique<webrtc::RtcEventLogFactory>(_pcf_deps.task_queue_factory.get());
  _pcf_deps.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
  _pcf_deps.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
  _pcf_deps.adm = rtc::scoped_refptr<webrtc::AudioDeviceModule>(FakeAudioCaptureModule::Create());
  _pcf_deps.audio_processing = apm; // Gets moved in EnableMedia.

  webrtc::EnableMedia(_pcf_deps);

  _peerConnectionFactory = webrtc::CreateModularPeerConnectionFactory(std::move(_pcf_deps));
}

PcFactory::~PcFactory()
{
  for (auto pc : _peerConnections) {
    pc->Close();
  }
  _peerConnections.clear();
  _peerConnectionFactory = nullptr;
}

std::string PcFactory::CreatePeerConnection(const char* buffer, int length) {

  std::string offerStr(buffer, length);

  std::cout << offerStr << std::endl;

  auto offerJson = nlohmann::json::parse(offerStr);

  //std::cout << "CreatePeerConnection on thread " << std::this_thread::get_id() << " supplied with offer : " << std::endl << offerJson.dump() << std::endl;
  std::cout << "CreatePeerConnection on thread " << std::this_thread::get_id() << " supplied with offer : " << std::endl;

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  //config.media_config.audio = new cricket::MediaConfig::Audio();
  //config.continual_gathering_policy = webrtc::PeerConnectionInterface::ContinualGatheringPolicy::GATHER_ONCE;
  
  auto dependencies = webrtc::PeerConnectionDependencies(new PcObserver());

  auto pcOrError = _peerConnectionFactory->CreatePeerConnectionOrError(config, std::move(dependencies));

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc = nullptr;

  if (!pcOrError.ok()) {
    std::cerr << "Failed to get peer connection from factory. " << pcOrError.error().message() << std::endl;
    return "error";
  }
  else {
    pc = pcOrError.MoveValue();

    // Create a local audio source
    rtc::scoped_refptr<webrtc::AudioSourceInterface> audio_source =
      _peerConnectionFactory->CreateAudioSource(cricket::AudioOptions());

    // Create a local audio track
    rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track =
      _peerConnectionFactory->CreateAudioTrack("audio_label", audio_source.get());

    if (!audio_track) {
      std::cerr << "Failed to create AudioTrack." << std::endl;
      return "error";
    }

    std::cout << "AudioTrack created successfully." << std::endl;

    auto sender = pc->AddTrack(audio_track, { "stream_id" });

    if (!sender.ok()) {
      std::cerr << "Failed to add AudioTrack to PeerConnection." << std::endl;
      return "error";
    }

    std::cout << "AudioTrack added to PeerConnection successfully." << std::endl;

    //webrtc::DataChannelInit config;
    //auto dc = pc->CreateDataChannel("data_channel", &config);

    _peerConnections.push_back(pc);

    webrtc::SdpParseError sdpError;
    auto remoteOffer = webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, offerJson["sdp"], &sdpError);

    if (remoteOffer == nullptr) {
      std::cerr << "Failed to get parse remote SDP. " << sdpError.description << std::endl;
      return "error";
    }
    else {

      std::mutex mtx;
      std::condition_variable cv;
      bool isReady = false;

      SignalingThread->PostTask([&]() {
        std::cout << "Setting remote description on peer connection, thread ID " << std::this_thread::get_id() << std::endl;

        pc->SetRemoteDescription(remoteOffer->Clone(), SetRemoteSdpObserver::Create());

        std::cout << "SetLocalDescription on thread, thread ID " << std::this_thread::get_id() << std::endl;
        pc->SetLocalDescription(CreateSdpObserver::Create(mtx, cv, isReady));
      });

      std::unique_lock<std::mutex> lck(mtx);

      if (!isReady) {
        std::cout << "Waiting for create answer to complete..." << std::endl;

        bool completed = cv.wait_for(lck, std::chrono::seconds(SET_REMOTE_SDP_TIMEOUT_SECONDS), [&]() {
          return isReady;
          });

        if (!completed) {
          std::cout << "Timed out waiting for isReady." << std::endl;
          return std::string();
        }
        else {
          std::cout << "isReady is now true within 3s." << std::endl;
        }
      }
      
      auto localDescription = pc->local_description();

      if (localDescription == nullptr) {
        return "Failed to set local description.";
      }
      else {
        std::cout << "Create answer complete." << std::endl;

        std::string answerSdp;
        localDescription->ToString(&answerSdp);

        std::cout << answerSdp << std::endl;

        nlohmann::json answerJson;
        answerJson["type"] = "answer";
        answerJson["sdp"] = answerSdp;

        return answerJson.dump();
      }
    }
  }
}
