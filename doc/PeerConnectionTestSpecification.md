## Overview

The purpose of the `Peer Connection Test` is verify that the WebRTC steps up to and including a DTLS handshake can be successfully completed between two peers: the `Server Peer` and the `Client Peer`.

## View the Code

The descriptions in the following sections outline how the `Server Peer` and `Client Peer` work together. Often it can be easier to read the code. A number of implementations in different languages that can be reviewed:

  - Python: [aiortc Test Server](../aiortc/server.py)
  - Python: [aiortc Test Client](../aiortc/client.py)
  - Go: [Pion Test Server](../pion/main.go)
  - Go: [Pion Test Client](../pion/client/main.go)
  - C#: [SIPSorcery Test Server](../sipsorcery/server/Program.cs)
  - C#: [SIPSorcery Test Client](../sipsorcery/client/Program.cs)
  - C++: [libdatachannel Test Server](../libdatachannel/server.cpp)
  - C++: [libdatachannel Test Client](../libdatachannel/client.cpp)
  - TypeScript: [werift Test Server](../werift/server.ts)
  - TypeScript: [werift Test Client](../werift/client.ts)

## Signaling

The single step signaling operation involved in the test is a HTTP POST request from the `Client Peer` to the `Server Peer`. The Client must send its SDP offer to the Server and the Server will respond with its SDP answer.

**Note**: Using this single-shot signaling approach means that at least one of the `Client Peer` or `Server Peer` must operate in non-trickle ICE mode and include their ICE candidates in the SDP. Ideally both peers should be configured to operate in non-trickle mode. This is a deliberate choice in order to reduce the signaling complexity.

## Server Peer Operation

The required operation for a `Server Peer` is:

 - Listen on TCP port `8080` for HTTP POST requests. The URL that the HTTP server must listen on for POST requests is:
   - http://*:8080/offer (if a wildcard IP address cannot be used any address that the client can reach is suitable).
 - POST requests from the client will have a body that is a JSON encoded [RTCSessionDescriptionInit](https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit) object.
 - When an SDP offer is received create a new `Peer Connection` and set the remote offer.
 - Generate an SDP answer and return it as a JSON encoded [RTCSessionDescriptionInit](https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit) object in the HTTP POST response.
 - Perform the `Peer Connection` initialisation.

 ## Client Peer Operation

 The required operation for a `Client Peer` is:

  - Create a new `Peer Connection` and generate an SDP offer.
  - Send the SDP offer as a JSON encoded [RTCSessionDescriptionInit](https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit) object to the `Server Peer` with an HTTP POST request to:
    - http://localhost:8080/offer (adjust the address if the server is not on localhost)
  - The HTTP POST response will be an SDP answer as a JSON encoded [RTCSessionDescriptionInit](https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit) object.
  - Set the remote offer on the `Peer Connection`.
  - Perform the `Peer Connection` initialisation.
  - If the DTLS handshake is successfully completed the client should **Return 0**. If the test fails or times out the client should **Return 1**. It's solely up to the `Client Peer` to decide if the test was successful or not.

 In addition to the various implementations listed above a javascript application that functions as an Echo Test `Client Peer` is available [here](../html/index.html). 
