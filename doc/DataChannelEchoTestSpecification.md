## Overview

The purpose of the `Data Channel Echo Test` is to verify that data channel messages can be sent between  two peers: the `Server Peer` and the `Client Peer`.

This test builds on the [Peer Connection Test](doc/PeerConnectionTestSpecification.md) and uses the same signalling mechanism to exchange the SDP offer and answer.

## Server Peer Operation

The required operation for a `Server Peer` is:

 - Perform the same steps as outlined in the [Peer Connection Test](doc/PeerConnectionTestSpecification.md) to complete the DTLS handshake. An additional requirement is that a data channel will be offered in the `Client Peer` SDP and should be accepted.
 - Following the DTLS handshake the SCTP association and data channels should be created.
 - The `Server Peer` should then wait for a string message on the `Client Peer's` data channel. Once received it should respond by sending back an identical string on the same data channel the message was received on.

The `Server Peer` must be capable of being run in the same manner described in the [Echo Test Docker Requirements](doc/EchoTestDockerRequirements.md):

`docker run ghcr.io/sipsorcery/sipsorcery-webrtc-echo`

 ## Client Peer Operation

 The required operation for a `Client Peer` is:

 - Perform the same steps as outlined in the [Peer Connection Test](doc/PeerConnectionTestSpecification.md) to complete the DTLS handshake. An additional requirement is that a data channel must be offered in the SDP sent to the `Server Peer`.
 - Following the DTLS handshake the SCTP association and data channels should be created.
 - Once the data channel is opened a short (4 or 5 characters is sufficient) pseudo-random string should be sent to the `Server Peer`. 
 - If the `Server Peer` responds with an identical string on the same data channel the client should **Return 0** to indicate a successful test. If the `Server Peer` responds with a different message or the test times out the client should **Return 1**. It's solely up to the `Client Peer` to decide if the test was successful or not.

 The `Client Peer` docker image command line has been expanded from the [Echo Test Docker Requirements](doc/EchoTestDockerRequirements.md). Two additional command line parameters are required:

`-s`: The URL the `Server Peer` is listening on got the SDP offer.

`-t`: A number to indicate the test type. The purpose of this parameter is to allow the same client application to be used for multiple tests. For this test the parameter must be set to `1`.

`docker run ghcr.io/sipsorcery/sipsorcery-webrtc-echo -s <server url> -t 1`
