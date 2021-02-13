"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const tslib_1 = require("tslib");
const express_1 = tslib_1.__importDefault(require("express"));
const werift_1 = require("werift");
const yargs = tslib_1.__importStar(require("yargs"));
const https_1 = tslib_1.__importDefault(require("https"));
const fs_1 = require("fs");
const args = yargs
    .option("host", {
    description: "Host for HTTP server (default: 0.0.0.0)",
    default: "0.0.0.0",
})
    .option("port", {
    description: "Port for HTTP server (default: 8080)",
    default: 8080,
})
    .option("cert-file", { description: "SSL certificate file (for HTTPS)" })
    .option("key-file", { description: "SSL key file (for HTTPS)" })
    .help().argv;
const app = express_1.default();
app.use(express_1.default.json());
if (args["cert-file"] && args["key-file"]) {
    https_1.default
        .createServer({
        cert: fs_1.readFileSync(args["cert-file"]),
        key: fs_1.readFileSync(args["key-file"]),
    }, app)
        .listen(args.port, args.host);
}
else {
    app.listen(args.port, args.host);
}
app.use(express_1.default.static("../aiortc"));
app.post("/offer", (req, res) => tslib_1.__awaiter(void 0, void 0, void 0, function* () {
    const offer = req.body;
    const pc = new werift_1.RTCPeerConnection({
        iceConfig: { stunServer: ["stun.l.google.com", 19302] },
    });
    pc.onTransceiver.subscribe((transceiver) => tslib_1.__awaiter(void 0, void 0, void 0, function* () {
        const [track] = yield transceiver.onTrack.asPromise();
        track.onRtp.subscribe((rtp) => {
            transceiver.sendRtp(rtp);
        });
    }));
    yield pc.setRemoteDescription(offer);
    const answer = yield pc.setLocalDescription(pc.createAnswer());
    res.send(answer);
}));
