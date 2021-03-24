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
  void OnSetRemoteDescriptionComplete(webrtc::RTCError error)
  {
    std::cout << "OnSetRemoteDescriptionComplete ok ? " << std::boolalpha << error.ok() << "." << std::endl;
  }
};

class CreateSdpObserver :
  public webrtc::SetLocalDescriptionObserverInterface
{
public:
  
  CreateSdpObserver(std::mutex& mtx, std::condition_variable& cv, bool& isReady)
    : _mtx(mtx), _cv(cv), _isReady(isReady) {
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

#endif