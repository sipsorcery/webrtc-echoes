**Description**

A Rust application that runs a WebRTC Echo Server.

**Usage**

By default the in built web server will listen on `http://*:8080/`.

 - `cd webrtc-rs/server`
 - `cargo run` or `cargo build && ./target/debug/server`

POST an SDP offer to `http://*:8080/offer` or open `http://localhost:8080/` in a browser to use the included `index.html` demo page.

```
An example of webrtc-rs echo server.

USAGE:
    server [FLAGS] [OPTIONS]

FLAGS:
        --fullhelp    Prints more detailed help information
    -d, --debug       Prints debug log information
    -h, --help        Prints help information
    -V, --version     Prints version information

OPTIONS:
        --host <host>              Echo HTTP server is hosted on. [default: 0.0.0.0:8080]
        --html-file <html-file>    Html file to be displayed. [default: ../../html/index.html]
```