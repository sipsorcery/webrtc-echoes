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
*
* License: Public Domain (no warranty, use at own risk)
/******************************************************************************/

#include "PcFactory.h"
#include "json.hpp"

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

#include <iostream>
#include <sstream>

PcFactory::PcFactory() :
  _peerConnections()
{  
  //webrtc::PeerConnectionFactoryDependencies _pcf_deps;
  //_pcf_deps.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
  //_pcf_deps.signaling_thread = rtc::Thread::Create().release();
  //_pcf_deps.network_thread = rtc::Thread::CreateWithSocketServer().release();
  //_pcf_deps.worker_thread = rtc::Thread::Create().release();
  //_pcf_deps.call_factory = webrtc::CreateCallFactory();
  //_pcf_deps.event_log_factory = std::make_unique<webrtc::RtcEventLogFactory>(
  //_pcf_deps.task_queue_factory.get());

  //cricket::MediaEngineDependencies media_deps;
  //media_deps.task_queue_factory = _pcf_deps.task_queue_factory.get();
  //media_deps.adm = webrtc::AudioDeviceModule::CreateForTest(
  //  webrtc::AudioDeviceModule::kDummyAudio, _pcf_deps.task_queue_factory.get());
  //media_deps.audio_encoder_factory =
  //  webrtc::CreateAudioEncoderFactory<webrtc::AudioEncoderG711>();
  //media_deps.audio_decoder_factory =
  //  webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderG711>();
  //media_deps.video_encoder_factory = webrtc::CreateBuiltinVideoEncoderFactory();
  //media_deps.video_decoder_factory = webrtc::CreateBuiltinVideoDecoderFactory();
  //media_deps.audio_processing = webrtc::AudioProcessingBuilder().Create();

  //_pcf_deps.media_engine = cricket::CreateMediaEngine(std::move(media_deps));

  //_peerConnectionFactory = webrtc::CreatePeerConnectionFactory(
  //  rtc::Thread::CreateWithSocketServer().release()/* network_thread */,
  //  rtc::Thread::Create().release() /* worker_thread */,
  //  rtc::Thread::Create().release() /* signaling_thread */,
  //  webrtc::AudioDeviceModuleForTest::CreateForTest(webrtc::AudioDeviceModule::AudioLayer::kDummyAudio, webrtc::CreateDefaultTaskQueueFactory().release()),
  //  webrtc::CreateAudioEncoderFactory<webrtc::AudioEncoderG711>(), //webrtc::CreateBuiltinAudioEncoderFactory(),
  //  webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderG711>(), //webrtc::CreateBuiltinAudioDecoderFactory(),
  //  webrtc::CreateBuiltinVideoEncoderFactory(),
  //  webrtc::CreateBuiltinVideoDecoderFactory(),
  //  nullptr /* audio_mixer */,
  //  webrtc::AudioProcessingBuilder().Create() /* audio_processing */);

  //_peerConnectionFactory = webrtc::CreateModularPeerConnectionFactory(std::move(_pcf_deps));

  _networkThread = rtc::Thread::CreateWithSocketServer();
  _networkThread->Start();
  _workerThread = rtc::Thread::Create();
  _workerThread->Start();
  _signalingThread = rtc::Thread::Create();
  _signalingThread->Start();

  _fakeAudioCapture = FakeAudioCaptureModule::Create();

  _peerConnectionFactory = webrtc::CreatePeerConnectionFactory(
    _networkThread.get() /* network_thread */,
    _workerThread.get() /* worker_thread */,
    _signalingThread.get() /* signaling_thread */,
    //nullptr /* default_adm */,
    //webrtc::AudioDeviceModuleForTest::CreateForTest(webrtc::AudioDeviceModule::AudioLayer::kDummyAudio, webrtc::CreateDefaultTaskQueueFactory().release()),
    rtc::scoped_refptr<webrtc::AudioDeviceModule>(_fakeAudioCapture),
    webrtc::CreateAudioEncoderFactory<webrtc::AudioEncoderG711>(),
    webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderG711>(),
    webrtc::CreateBuiltinVideoEncoderFactory(),
    webrtc::CreateBuiltinVideoDecoderFactory(),
    nullptr /* audio_mixer */,
    nullptr); //webrtc::AudioProcessingBuilder().Create() /* audio_processing */);
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
  auto offerJson = nlohmann::json::parse(offerStr);

  std::cout << offerJson.dump() << std::endl;

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  config.enable_dtls_srtp = true;

  auto observer = new rtc::RefCountedObject<PcObserver>();

  auto pcOrError = _peerConnectionFactory->CreatePeerConnectionOrError(
    config, webrtc::PeerConnectionDependencies(observer));

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc = nullptr;

  if (!pcOrError.ok()) {
    std::cerr << "Failed to get peer connection from factory. " << pcOrError.error().message() << std::endl;
    return "error";
  }
  else {
    pc = pcOrError.MoveValue();

    _peerConnections.push_back(pc);

    webrtc::SdpParseError sdpError;
    auto remoteOffer = webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, offerJson["sdp"], &sdpError);

    if (remoteOffer == nullptr) {
      std::cerr << "Failed to get parse remote SDP." << std::endl;
      return "error";
    }
    else {
      std::cout << "Setting remote description on peer connection." << std::endl;
      auto setRemoteObserver = new rtc::RefCountedObject<SetRemoteSdpObserver>();
      pc->SetRemoteDescription(remoteOffer->Clone(), setRemoteObserver);

      std::mutex mtx;
      std::condition_variable cv;
      bool isReady = false;

      auto createObs = new rtc::RefCountedObject<CreateSdpObserver>(mtx, cv, isReady);
      pc->SetLocalDescription(createObs);

      std::unique_lock<std::mutex> lck(mtx);

      if (!isReady) {
        std::cout << "Waiting for create answer to complete..." << std::endl;
        cv.wait(lck);
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
