import { RTCPeerConnection, RtpPacket } from "werift";
import axios from "axios";
import { createSocket } from "dgram";
import { exec } from "child_process";

const url = process.argv[2] || "http://localhost:8080/offer";

const udp = createSocket("udp4");
udp.bind(5000);

new Promise<void>(async (r, f) => {
  setTimeout(() => {
    f();
  }, 30_000);

  const pc = new RTCPeerConnection({
    iceConfig: { stunServer: ["stun.l.google.com", 19302] },
  });
  const transceiver = pc.addTransceiver("video", "sendrecv");
  transceiver.onTrack.once((track) => {
    track.onRtp.subscribe((rtp) => {
      console.log(rtp.header);
      r();
    });
  });

  await pc.setLocalDescription(await pc.createOffer()).catch((e) => f(e));
  const { data } = await axios.post(url, pc.localDescription).catch((e) => {
    f(e);
    throw e;
  });
  console.log("server answer sdp", data?.sdp);
  pc.setRemoteDescription(data).catch((e) => f(e));

  await pc.connectionStateChange.watch((state) => state === "connected");
  r();

  // const payloadType = transceiver.codecs.find((codec) =>
  //   codec.mimeType.toLowerCase().includes("vp8")
  // )!.payloadType;

  // udp.on("message", (data) => {
  //   const rtp = RtpPacket.deSerialize(data);
  //   rtp.header.payloadType = payloadType;
  //   transceiver.sendRtp(rtp);
  // });

  // exec(
  //   "gst-launch-1.0 videotestsrc ! video/x-raw,width=640,height=480,format=I420 ! vp8enc error-resilient=partitions keyframe-max-dist=10 auto-alt-ref=true cpu-used=5 deadline=1 ! rtpvp8pay ! udpsink host=127.0.0.1 port=5000"
  // );
})
  .then(() => {
    console.log("done");
    process.exit(0);
  })
  .catch((e) => {
    console.log("failed", e);
    process.exit(1);
  });
