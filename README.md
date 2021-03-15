## Overview

This project aims to be a convenient location for [WebRTC](https://www.w3.org/TR/webrtc/) library developers to perform interoperability tests.

## Who can Participate

The project is open to everyone.

The developers likely to interested are those involved in WebRTC projects.

## Participating Libraries

The libraries that currently have a Client and Server implementation are:

 - [aiortc](https://github.com/aiortc/aiortc): WebRTC and ORTC implementation for Python using asyncio.
 - [Pion](https://github.com/pion/webrtc): Pure Go implementation of the WebRTC API.
 - [SIPSorcery](https://github.com/sipsorcery-org/sipsorcery) - A WebRTC, SIP and VoIP library for C# and .NET Core. Designed for real-time communications apps.
 - [werift-webrtc](https://github.com/shinyoshiaki/werift-webrtc): WebRTC Implementation for TypeScript (Node.js)

The libraries that currently have a Server implementation are:

  - [gstreamer](https://gstreamer.freedesktop.org/).
  - [libwebrtc](https://webrtc.googlesource.com/src/).

## Interoperability Tests

WebRTC covers a plethora of different standards and protocols. On top of that signaling is left up to each individual application.

The initial goal of this project is to find the simplest useful interoperability test so as to encourage an initial implementation for as many projects as possible.

 - **[Echo Test](doc/EchoTestSpecification.md)**: The first test is an Echo Test Server and/or Client that tests the ability to negotiate a peer connection up to the DTLS handshake. **A description of how the Echo Test works is available [here](doc/EchoTestSpecification.md)**.

![Echo Test Results](test/echo_test_results.png)

## Get Started

If you are interested in adding a library to this project the recommended steps are listed below. The steps don't necessarily have to be completed in any specific order.

 - Write an Echo Test Client application according to the [specification](doc/EchoTestSpecification.md#client-peer-operation) or base it off an [existing application](doc/EchoTestSpecification.md#view-the-code).

 - Test your client by building and running one of the [Echo Test Servers](https://github.com/sipsorcery/webrtc-echoes/blob/master/doc/EchoTestSpecification.md#view-the-code) or you can use one of the [Echo Test Server Docker Images](https://github.com/sipsorcery?tab=packages&q=webrtc):

````
docker pull ghcr.io/sipsorcery/sipsorcery-webrtc-echo:latest
docker run -it --rm -p 8080:8080 ghcr.io/sipsorcery/sipsorcery-webrtc-echo
````

- If you encounter any problems open an [Issue](https://github.com/sipsorcery/webrtc-echoes/issues). When done submit a [Pull Request](https://github.com/sipsorcery/webrtc-echoes/pulls) for your application.

- Repeat the process for an [Echo Test Server](doc/EchoTestSpecification.md#server-peer-operation).

- Create a [Dockerfile](doc/EchoTestDockerRequirements.md) and add a Pull Request for it so your Echo Test application(s) can be included in the automated testing.
