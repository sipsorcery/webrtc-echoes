import { RTCPeerConnection } from "werift";
import axios from "axios";
import { createSocket } from "dgram";

const url = process.argv[2] || "http://localhost:8080/offer";

const udp = createSocket("udp4");
udp.bind(5000);

new Promise<void>(async (r, f) => {
  const pc = new RTCPeerConnection({
    iceConfig: { stunServer: ["stun.l.google.com", 19302] },
  });
  const transceiver = pc.addTransceiver("video", "sendrecv");

  await pc.setLocalDescription(pc.createOffer());
  const { data } = await axios.post(url, pc.localDescription);
  pc.setRemoteDescription(data);

  await transceiver.sender.onReady.asPromise();
  r();

  setTimeout(() => {
    f();
  }, 30_000);
})
  .then(() => {
    console.log("done");
    process.exit(0);
  })
  .catch(() => {
    console.log("failed");
    process.exit(1);
  });
