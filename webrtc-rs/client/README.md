**Description**

A Rust application that acts as a peer for a WebRTC Echo Server.

**Usage**

By default the in built client will attempt to POST its SDP offer to an echo server at `http://localhost:8080/`.

 - Make sure the echo test server is running.
 - `cd webrtc-rs/client`
 - `cargo run` or `cargo build && ./target/debug/client`
 - For more details, run './target/debug/client -h'
```
An example of webrtc-rs echo client.

USAGE:
    client [FLAGS] [OPTIONS]

FLAGS:
        --fullhelp    Prints more detailed help information
    -d, --debug       Prints debug log information
    -h, --help        Prints help information
    -V, --version     Prints version information

OPTIONS:
        --server <server>    Echo HTTP server is hosted on. [default: localhost:8080]
```