/******************************************************************************
* Filename: PcObserver.h
*
* Description:
* Observer to receive notifications of peer connection lifetime events.
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

#ifndef __PEER_CONNECTION_OBSERVER__
#define __PEER_CONNECTION_OBSERVER__

#include <api/peer_connection_interface.h>

#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

class PcObserver :
  public webrtc::PeerConnectionObserver
{ 
public:
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state);
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel);
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state);
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
  void OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams);
  void OnTrack(
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver);
  void OnConnectionChange(
    webrtc::PeerConnectionInterface::PeerConnectionState new_state);
};

class SetRemoteSdpObserver :
  public webrtc::SetRemoteDescriptionObserverInterface
{
public: 
  static rtc::scoped_refptr<SetRemoteSdpObserver> Create() {
    return rtc::scoped_refptr<SetRemoteSdpObserver>(new rtc::RefCountedObject<SetRemoteSdpObserver>());
  }

  void OnSetRemoteDescriptionComplete(webrtc::RTCError error)
  {
    std::cout << "OnSetRemoteDescriptionComplete ok ? " << std::boolalpha << error.ok() << "." << std::endl;
  }
};

class CreateSdpObserver :
  public webrtc::SetLocalDescriptionObserverInterface
{
public:

  static rtc::scoped_refptr<CreateSdpObserver> Create(std::mutex& mtx, std::condition_variable& cv, bool& isReady) {
    return rtc::scoped_refptr<CreateSdpObserver>(new rtc::RefCountedObject<CreateSdpObserver>(mtx, cv, isReady));
  }
  
  CreateSdpObserver(std::mutex& mtx, std::condition_variable& cv, bool& isReady)
    : _mtx(mtx), _cv(cv), _isReady(isReady) {
    std::cout << "CreateSdpObserver Constructor." << std::endl;
  }

  ~CreateSdpObserver() {
    std::cout << "CreateSdpObserver Destructor." << std::endl;
  }

  void OnSetLocalDescriptionComplete(webrtc::RTCError error) {
    std::cout << "OnSetLocalDescriptionComplete." << std::endl;

    if (!error.ok()) {
      std::cerr << "OnSetLocalDescription error. " << error.message() << std::endl;
    }

    std::unique_lock<std::mutex> lck(_mtx);
    _isReady = true;
    _cv.notify_all();
  }

private:
  std::mutex& _mtx;
  std::condition_variable& _cv;
  bool& _isReady;
};

class SetRemoteDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:

  static rtc::scoped_refptr<SetRemoteDescriptionObserver> Create() {
    return rtc::scoped_refptr<SetRemoteDescriptionObserver>(new rtc::RefCountedObject<SetRemoteDescriptionObserver>());
  }

  SetRemoteDescriptionObserver() {
    std::cout << "SetRemoteDescriptionObserver Constructor." << std::endl;
  }

  void OnSuccess() override {
    std::cout << "SetRemoteDescriptionObserver::OnSuccess" << std::endl;
  }

  void OnFailure(webrtc::RTCError error) override {
    std::cerr << "SetRemoteDescriptionObserver::OnFailure: " << error.message() << std::endl;
  }
};

class SetLocalDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:

  static rtc::scoped_refptr<SetLocalDescriptionObserver> Create() {
    return rtc::scoped_refptr<SetLocalDescriptionObserver>(new rtc::RefCountedObject<SetLocalDescriptionObserver>());
  }

  void OnSuccess() override {
    std::cout << "SetLocalDescriptionObserver::OnSuccess" << std::endl;
  }

  void OnFailure(webrtc::RTCError error) override {
    std::cerr << "SetLocalDescriptionObserver::OnFailure: " << error.message() << std::endl;
  }
};

class CreateAnswerObserver : public webrtc::CreateSessionDescriptionObserver {
public:

  static std::unique_ptr<CreateAnswerObserver> Create() {
    return std::unique_ptr<CreateAnswerObserver>();
  }

  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
    std::cout << "CreateAnswerObserver::OnSuccess" << std::endl;
  }

  void OnFailure(webrtc::RTCError error) override {
    std::cerr << "CreateAnswerObserver::OnFailure: " << error.message() << std::endl;
  }
};

#endif