**Description**

A Go application that runs a WebRTC Echo Server.

**Prerequisites**

 - Install teh Go language SDK https://golang.org/doc/install

**Usage**

By default the in built web server will listen on `http://*:8080/`.

`go run main.go`

POST an SDP offer to `http://*:8080/offer` or open `http://localhost:8080/` in a browser to use the included `index.html` demo page.
