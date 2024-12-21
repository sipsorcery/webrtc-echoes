/*
* Filename: PcFactory.h
*
* Description:
* Factory to allow the creation of new WebRTC peer connection instances.
*
* Author:
* Aaron Clauson (aaron@sipsorcery.com)
*
* History:
* 08 Mar 2021	Aaron Clauson	  Created, Dublin, Ireland.
* 21 Dec 2024 Aaron Clauson   Updated for libwebrtc version m132.
*
* License: Public Domain (no warranty, use at own risk)
*/

#ifndef __PEER_CONNECTION_FACTORY__
#define __PEER_CONNECTION_FACTORY__

#include "PcObserver.h"

#include <api/peer_connection_interface.h>
#include <api/scoped_refptr.h>
#include <media/engine/webrtc_media_engine.h>
#include "absl/base/nullability.h"
#include "api/environment/environment.h"
#include "call/call.h"
#include "call/call_config.h"
#include "pc/media_factory.h"
#include "rtc_base/checks.h"

#include <condition_variable>
#include <memory>
#include <string>
#include <vector>

class PcFactory {
public:
  PcFactory();
  ~PcFactory();
  std::string CreatePeerConnection(const char* buffer, int length);

  /* The thread logic is now tricky. I was not able to get even a basic peer connection
  * example working on Windows in debug mode due to the failing thread checks, see
  * https://groups.google.com/u/2/g/discuss-webrtc/c/HG9hzDP2djA
// From peer_connection_interface.h line 1632 
// If `network_thread` or `worker_thread` are null, the PeerConnectionFactory
// will create the necessary thread internally. If `signaling_thread` is null,
// the PeerConnectionFactory will use the thread on which this method is called
// as the signaling thread, wrapping it in an rtc::Thread object if needed.
  */
  std::unique_ptr<rtc::Thread> SignalingThread;

private:
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _peerConnectionFactory;
  std::vector<rtc::scoped_refptr<webrtc::PeerConnectionInterface>> _peerConnections;
};

#endif