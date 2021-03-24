## Building docker image for static gstreamer build

`docker build -t gstreamer-builder:0.1 -f Dockerfile-Gstreamerbuilder --progress=plain .`

## Building docker image

`docker build -t gstreamer-webrtc-echo:0.x --progress=plain .`

## Running docker image

`docker run -it --init --rm -p 8080:8080 ghcr.io/sipsorcery/gstreamer-webrtc-echo:latest`

Set a gstreamer environment variable for additional logging:

`docker run -it --init --rm -p 8080:8080 -e "GST_DEBUG=4,dtls*:7" ghcr.io/sipsorcery/gstreamer-webrtc-echo:latest`

Override the gstreamer-echo-app and start a bash shell plus add a local volume mapping:

`docker run -it -p 8080:8080 -v %cd%:/pcdodo --entrypoint /bin/bash ghcr.io/sipsorcery/gstreamer-webrtc-echo:latest`
