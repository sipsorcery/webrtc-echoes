# WebRTC Echo Server

**Description**

A Node Typescript application that runs a WebRTC Echo Server.

**Prerequisites**

- Node.js version 14+
- yarn (or npm)
- `yarn install` (or `npm install`)

**Usage**

By default the in built web server will listen on `http://*:8080/`.

`yarn server` (or `npm run server`)

POST an SDP offer to `http://*:8080/offer` or open `http://localhost:8080/` in a browser to use the included `index.html` demo page.

# WebRTC Echo Client

**Description**

A Node Typescript application that acts as a peer for a WebRTC Echo Server.

**Prerequisites**

- ffmpeg
- GStreamer
- Node.js version 14+
- yarn (or npm)
- `yarn install` (or `npm install`)

**Usage**

By default the in built client will attempt to POST its SDP offer to an echo server at `http://localhost:8080/`.

- Make sure the echo test server is running.
- `ffmpeg -re -f lavfi -i testsrc=size=640x480:rate=30 -vcodec libvpx -keyint_min 30 -cpu-used 5 -deadline 1 -g 10 -error-resilient 1 -auto-alt-ref 1 -f rtp rtp://127.0.0.1:5000`
- `gst-launch-1.0 -v udpsrc port=4002 caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)VP8, payload=(int)97" ! rtpvp8depay ! decodebin ! videoconvert ! autovideosink`
- `yarn client` (or `npm run client`)
