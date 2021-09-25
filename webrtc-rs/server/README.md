**Description**

A Rust application that runs a WebRTC Echo Server.

**Usage**

By default the in built web server will listen on `http://*:8080/`.

 - `cd webrtc-rs/server`
 - `cargo run` or `cargo build && ./target/debug/server`

POST an SDP offer to `http://*:8080/offer` or open `http://localhost:8080/` in a browser to use the included `index.html` demo page.
