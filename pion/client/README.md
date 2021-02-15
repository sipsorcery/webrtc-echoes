**Description**

A Go application that acts as a peer for a WebRTC Echo Server.

**Prerequisites**

 - Install the Go language SDK https://golang.org/doc/install

**Usage**

By default the in built client will attempt to POST its SDP offer to an echo server at `http://localhost:8080/`.

 - Make sure the echo test server is running.
 - `go run main.go` or `go build main.go && main.exe`
 