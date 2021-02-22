## Overview

The purpose of the WebRTC Echo Test is verify that a Peer Connection can be established between two peers: the `Server Peer` and the `Client Peer`.

The "Echo" part of the test is the ability of the `Server Peer` to reflect any audio or video packets it receives back to the `Client Peer`. This is particularly useful when testing when a Browser is in the `Client Peer` role as it can provide a prompt visual indication of the success of the test.

## View the Code

The descriptions in the following sections outline how the Echo Test Server and Client work. Often it can be easier to read the code and at the time of writing there are already a number of implementations in different languages that can be reviewed.

  - Python: [aiortc Echo Test Server](../aiortc/server.py)
  - Go: [Pion Echo Test Server](../pion/main.go)
  - Go: [Pion Echo Test Client](../pion/client/main.go)
  - C#: [SIPSorcery Echo Test Server](../sipsorcery/server/Program.cs)
  - C#: [SIPSorcery Echo Test Client](../sipsorcery/client/Program.cs)
  - TypeScript: [werift Echo Test Server](../werift/server.ts)
  - TypeScript: [werift Echo Test Client](../werift/client.ts)

## Signaling

The single signaling operation involved in the test is a HTTP POST request from the `Client Peer` to the `Server Peer`. The Client must send it's SDP offer to the Server and the Server will respond with its SDP answer.

**Note**: Using this single-shot signaling approach means that at least one of the `Client Peer` or `Server Peer` must operate in non-trickle ICE mode and include their ICE candidates in the SDP. Ideally both peers should be configured to operate in non-trickle mode. This is a deliberate choice in order to reduce the signaling complexity.

## Server Peer Operation

The required flow for a `Server Peer` is:

 - Listen on TCP port `8080` for HTTP POST requests. The URL that the HTTP server must listen on for POST requests is:
   - http://*:8080/offer (if a wildcard IP address cannot be used any address that the client can reach is suitable).
 - POST requests from the client will have a body that is a JSON encoded [RTCSessionDescriptionInit](https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit) object.
 - When an SDP offer is received create a new `Peer Connection` and set the remote offer.
 - Generate an SDP answer and return it as a JSON encoded [RTCSessionDescriptionInit](https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit) object in the HTTP POST response.
 - Perform the `Peer Connection` initialisation.
 - For any RTP media received on the `Peer Connection` send it back to the remote peer.

 ## Client Peer Operation

 The required flow for a `Client Peer` is:

  - Create a new `Peer Connection` and generate an SDP offer.
  - Send the SDP offer as a JSON encoded [RTCSessionDescriptionInit](https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit) object to the `Server Peer` with an HTTP POST request to:
    - http://localhost:8080/offer (adjust the address if the server is not on localhost)
  - The HTTP POST response will be an SDP answer as a JSON encoded [RTCSessionDescriptionInit](https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit) object.
  - Set the remote offer on the `Peer Connection`.
  - Perform the `Peer Connection` initialisation.
  - Optionally send any audio or video to the server and have it echoed back.
  - Once the client has determined the `Peer Connection` has been successfully connected it shoudl close the connetion adn **Return 0**. If the connection fails or times out the client should **Return 1**.

 In addition to the various implementations listed above a javascript application that functions as an Echo Test `Client Peer` is available [here](../html/index.html). 
