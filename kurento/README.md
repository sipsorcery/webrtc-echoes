## Building the kurento webrtc echo test docker image

`docker build -t kurento-webrtc-echo:0.1 --progress=plain .`

## Running docker image

`docker run -it --init --rm -p 8080:8080 kurento-webrtc-echo:0.1`

## Building dotnet app

For some as yet unknown reason `dotnet` freezes on first run when installed on the `kurento/kurento-media-server-dev` image. The alternative is to publish the application and copy across the binaries.

The publish command to build the app is:

`dotnet publish -r linux-x64 --self-contained`

** This publish command still produces a long list of individual files. Using the publish profile in Visual Studio is able to produce a single file. Work out how to do this from the command line.
