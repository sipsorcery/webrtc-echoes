import { RTCPeerConnection } from "werift";
import axios from "axios";
import { createSocket } from "dgram";

const url = process.argv[2] || "http://localhost:8080/offer";

const udp = createSocket("udp4");
udp.bind(5000);

new Promise<any>(async (r, f) => {
  setTimeout(() => {
    f();
  }, 30_000);

  const pc = new RTCPeerConnection({
    iceConfig: { stunServer: ["stun.l.google.com", 19302] },
  });
  pc.addTransceiver("video", "sendrecv");

  await pc.setLocalDescription(await pc.createOffer()).catch((e) => f(e));
  const { data } = await axios.post(url, pc.localDescription).catch((e) => {
    f(e);
    throw e;
  });
  pc.setRemoteDescription(data).catch((e) => f(e));

  pc.connectionStateChange.watch((state) => state === "connected").then(r);
})
  .then(() => {
    console.log("done");
    process.exit(0);
  })
  .catch((e) => {
    console.log("failed", e);
    process.exit(1);
  });
