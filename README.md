## Overview

This project aims to be a convenient location for [WebRTC](https://www.w3.org/TR/webrtc/) library developers to perform interoperability tests.

## Who can Participate

The project is open to everyone.

The developers likely to interested are those involved in WebRTC projects.

## Participating Libraries

The libraries that currently have a WebRTC Echo Test Server and/or Client implementation are:

 - [aiortc](https://github.com/aiortc/aiortc): WebRTC and ORTC implementation for Python using asyncio.
 - [Pion](https://github.com/pion/webrtc): Pure Go implementation of the WebRTC API.
 - [SIPSorcery](https://github.com/sipsorcery-org/sipsorcery) - A WebRTC, SIP and VoIP library for C# and .NET Core. Designed for real-time communications apps.
 - [werift-webrtc](https://github.com/shinyoshiaki/werift-webrtc): WebRTC Implementation for TypeScript (Node.js)

 Coming Soon:

  - [libdatachannel](https://github.com/paullouisageneau/libdatachannel): C/C++ WebRTC Data Channels and Media Transport standalone library.

## Interoperability Tests

WebRTC covers a plethora of different standards and protocols. On top of that signaling is left up to each individual application.

The initial goal of this project is to find the simplest useful interoperability test so as to encourage an initial implementation for as many projects as possible.

 - **[Echo Test](doc/EchoTestSpecification.md)**: The first test is an Echo Test Server and/or Client that allows relatively simple testing between libraries and also between the Echo Test Servers and a WebRTC Browser. **The specification that describes how the Echo Test works is available [here](doc/EchoTestSpecification.md)**.

It's likely additional interoperability tests will be added in the future. New implementations will be encouraged to provide an Echo Test implementation first. This reduces the amount of work for a new library to participate and will also serve as a solid foundation for any subsequent tests.

## Manual Echo Test Results

| Server     | Client |         |            |          |         |         |
| ---------- | ------ | ------- | ---------- | -------- | ------- | ------- |
|            | aiortc | pion    | sipsorcery | werift   | chrome  | firefox |
| aiortc     |        | &#9745; | &#9745;    | &#9745;  | &#9745; | &#9745; |
| pion       |        | &#9745; | &#9745;    | &#9745;  | &#9745; | &#9745; |
| sipsorcery |        | &#9745; | &#9745;    | &#x2612; | &#9745; | &#9745; |
| werift     |        | &#9745; | &#x2612;   | &#9745;  | &#9745; | &#9745; |

## Automated Testing

Automated testing is implemented for libraries that have an associated Docker image using a [GitHub Actions Workflow](https://github.com/sipsorcery/webrtc-echoes/actions/workflows/interop-peerconnection-echo.yml). The workflow tests each library's Echo Test Server and/or Client against every other library's.

**[Automated Test Results](test/echo_test_results.md)**

**The requirements for an Echo Test Docker image are [here](doc/EchoTestDockerRequirements.md)**.

## Help Wanted

Implementations for any other libraries would be very welcome.

The libraries below have been identified as good candidates:

 - [gstreamer](https://gstreamer.freedesktop.org/).
 - [Google's WebRTC library](https://webrtc.googlesource.com/src/),
 - [webrtc-rs](https://github.com/webrtc-rs/webrtc): A pure Rust implementation of WebRTC API.
 - [Amazon's Kinesis Video](https://docs.aws.amazon.com/kinesisvideostreams-webrtc-dg/latest/devguide/what-is-kvswebrtc.html)
