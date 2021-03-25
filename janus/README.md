## Building the janus webrtc echo test docker image

`docker build -t janus-webrtc-echo:0.1 --progress=plain .`

## Running docker image

`docker run -it --init --rm -p 8080:8080 janus-webrtc-echo:0.1`

## Base Image

The canyanio janus gateway docker image is being used.

Docker image:

https://hub.docker.com/r/canyan/janus-gateway

Dockerfile:

https://github.com/canyanio/janus-gateway-docker/blob/master/Dockerfile


## Building dotnet app

As with the Kurento docker image For some as yet unknown reason `dotnet` freezes on first run when installed on the `canyan/janus-gateway:latest` image (at this point starting to suspect it's a problem between docker and WSL rather than the images). The alternative is to publish the application and copy across the binaries.

The publish command to build the app is:

`dotnet publish -r linux-x64 --self-contained`

** This publish command still produces a long list of individual files. Using the publish profile in Visual Studio is able to produce a single file. Work out how to do this from the command line.
