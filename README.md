## Overview

This project aims to be a convenient location for [WebRTC](https://www.w3.org/TR/webrtc/) library developers to perform interoperability tests.

## Who can Participate

The project is open to everyone.

The developers likely to interested are those involved in WebRTC projects.

## Participating Libraries

The libraries that currently have a Client and Server implementation are:

 - [aiortc](https://github.com/aiortc/aiortc): WebRTC and ORTC implementation for Python using asyncio.
 - [libdatachannel](https://github.com/paullouisageneau/libdatachannel): C/C++ WebRTC Data Channels and Media Transport standalone library (bindings for [Rust](https://github.com/lerouxrgd/datachannel-rs), [Node.js](https://github.com/murat-dogan/node-datachannel), and [Unity](https://github.com/hanseuljun/datachannel-unity))
 - [Pion](https://github.com/pion/webrtc): Pure Go implementation of the WebRTC API.
 - [webrtc-rs](https://github.com/webrtc-rs/webrtc): A pure Rust implementation of WebRTC stack. Rewrite [Pion](https://github.com/pion/webrtc) WebRTC stack in Rust.
 - [SIPSorcery](https://github.com/sipsorcery-org/sipsorcery): A WebRTC, SIP and VoIP library for C# and .NET. Designed for real-time communications apps.
 - [werift-webrtc](https://github.com/shinyoshiaki/werift-webrtc): WebRTC Implementation for TypeScript (Node.js)

Additional libraries/applications that currently have a Server implementation are:

  - [gstreamer](https://gstreamer.freedesktop.org/) `master` branch, commit ID `637b0d8dc25b660d3b05370e60a95249a5228a39` 20 Aug 2021 (gst-build commit ID `ebcca1e5ead27cab1eafc028332b1984c84b10b2` 26 Mar 2021).
  - [janus](https://janus.conf.meetecho.com/) version `0.10.7`, commit ID `04229be3eeceb28dbc57a70a57928aab223895a5`.
  - [kurento](https://www.kurento.org/) version `6.16.1~1.g907a859`.
  - [libwebrtc](https://webrtc.googlesource.com/src/) `m90` branch, commit ID `a4da76a880d31f012038ac721ac4abc7ea3ffa2d`, commit date `Fri Apr 9 21:03:39 2021 -0700`.

## Interoperability Tests

The current interoperability tests are:

 - **[Peer Connection Test](doc/PeerConnectionTestSpecification.md)**: The initial, and simplest, test is a WebRTC `Server Peer` and/or `Client Peer` that tests the ability to negotiate a peer connection up to a successful DTLS handshake. **A description of how the Peer Connection Test works is available [here](doc/PeerConnectionTestSpecification.md)**.

 - **[Data Channel Echo Test](doc/DataChannelEchoTestSpecification.md)**: This test builds on the [Peer Connection Test](doc/PeerConnectionTestSpecification.md) and adds a `data channel` test. It tests the ability of the peers to create a data channel and then checks that the `Server Peer` can echo a string message sent by the `Client Peer`.

## Peer Connection Test Results
Test run at 2024-10-26 21:07:42.806026

| Server       | aiortc | libdatachannel | pion | sipsorcery | werift |
|--------|--------|--------|--------|--------|--------|
| aiortc       | ✔      | ✔      | ✔      | ✔      | ✔      |
| gstreamer    | ✔      |        | ✔      | ✔      | ✔      |
| janus        | ✔      | ✔      | ✔      | ✔      | ✔      |
| kurento      | ✔      | ✔      |        | ✔      | ✔      |
| libdatachannel | ✔      | ✔      | ✔      | ✔      | ✔      |
| libwebrtc    | ✔      | ✔      | ✔      | ✔      | ✔      |
| pion         | ✔      | ✔      | ✔      | ✔      | ✔      |
| sipsorcery   | ✔      | ✔      | ✔      | ✔      | ✔      |
| werift       | ✔      | ✔      | ✔      | ✔      | ✔      |
## Data Channel Echo Test Results
Test run at 2024-10-26 21:03:29.247635

| Server       | libdatachannel | sipsorcery | werift |
|--------|--------|--------|--------|
| libdatachannel | ✔      | ✔      | ✔      |
| sipsorcery   | ✔      | ✔      | ✔      |
| werift       | ✔      | ✔      | ✔      |
## Get Started

If you are interested in adding a library to this project the recommended steps are listed below. The steps don't necessarily have to be completed in any specific order.

 - Write a Peer Connection Test Client application according to the [specification](doc/EchoTestSpecification.md#client-peer-operation) or base it off an [existing application](doc/EchoTestSpecification.md#view-the-code).

 - Test your client by building and running one of the [Peer Connection Test Servers](https://github.com/sipsorcery/webrtc-echoes/blob/master/doc/EchoTestSpecification.md#view-the-code) or you can use one of the [Peer Connection Test Server Docker Images](https://github.com/sipsorcery?tab=packages&q=webrtc):

````
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/aiortc-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/gstreamer-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/janus-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/kurento-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/libdatachannel-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/libwebrtc-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/pion-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/sipsorcery-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/webrtc-rs-webrtc-echo
docker run -it --rm --init -p 8080:8080 ghcr.io/sipsorcery/werift-webrtc-echo
````

- If you encounter any problems open an [Issue](https://github.com/sipsorcery/webrtc-echoes/issues). When done submit a [Pull Request](https://github.com/sipsorcery/webrtc-echoes/pulls) for your application.

- Repeat the process for a [Peer Connection Test Server](doc/PeerConnectionTestSpecification.md#server-peer-operation).

- Create a [Dockerfile](doc/EchoTestDockerRequirements.md) and add a Pull Request for it so your Peer Connection Test application(s) can be included in the automated testing.
