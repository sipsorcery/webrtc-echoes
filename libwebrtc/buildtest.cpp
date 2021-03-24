//#include <api/audio_codecs/builtin_audio_decoder_factory.h>
//#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
//#include <api/video_codecs/builtin_video_decoder_factory.h>
//#include <api/video_codecs/builtin_video_encoder_factory.h>

#include <iostream>

int main()
{
  std::cout << "webrtc native build test" << std::endl;

  //auto factory = webrtc::CreatePeerConnectionFactory(
  //  nullptr /* network_thread */,
  //  nullptr /* worker_thread */,
  //  nullptr /* signaling_thread */,
  //  nullptr /* default_adm */,
  //  nullptr, //webrtc::CreateBuiltinAudioEncoderFactory(),
  //  nullptr, //webrtc::CreateBuiltinAudioDecoderFactory(),
  //  nullptr, //webrtc::CreateBuiltinVideoEncoderFactory(),
  //  nullptr, //webrtc::CreateBuiltinVideoDecoderFactory(),
  //  nullptr /* audio_mixer */,
  //  nullptr /* audio_processing */,
  //  nullptr);

  //webrtc::PeerConnectionInterface::RTCConfiguration config;
  //auto pc = factory->CreatePeerConnection(config, nullptr, nullptr, nullptr);

  webrtc::SdpParseError sdpError;
  auto remoteOffer = webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, "", &sdpError);

  std::cout << "sdp parse error: " << sdpError.description << std::endl;

  std::cout << "Finished." << std::endl;

  return 0;
}
