**Description**

A Python console application that runs a WebRTC Echo Server.

**Prerequisites**

 - python3
 - `pip install aiohttp aiortc`

**Usage**

By default the in built web server will listen on `http://*:8080/`.

`python server.py`

POST an SDP offer to `http://*:8080/offer` or open `http://localhost:8080/` in a browser to use the included `index.html` demo page.

**Additional Usage**

The listening URL can be adjusted with a command line argument:

`python server.py --port 8081`
