## Overview

Once an `Server Peer` or `Client Peer` has succeeded in one or more manual tests with another implementation a Docker image should be created so that it can be included in the automated tests.

This document outlines the requirements for creating a Docker image.

## Example

The `Dockerfile` for the SIPSorcery library is available [here](../sipsorcery/Dockerfile).

````
FROM mcr.microsoft.com/dotnet/sdk:5.0 AS build
WORKDIR /src
COPY ["./sipsorcery/server", ""]
RUN dotnet publish "webrtc-echo.csproj" -c Release -o /app/publish
WORKDIR /src/client
COPY ["./sipsorcery/client", ""]
RUN dotnet publish "webrtc-echo-client.csproj" -c Release -o /app/publish/client

FROM mcr.microsoft.com/dotnet/runtime:5.0 AS final
WORKDIR /app
EXPOSE 8080
#EXPOSE 49000-65535
COPY ["html", "../html/"]
COPY ["./sipsorcery/client.sh", "/client.sh"]
RUN chmod +x /client.sh
COPY --from=build /app/publish .
COPY --from=build /app/publish/client .
ENTRYPOINT ["dotnet", "webrtc-echo.dll"]
````

## Requirements

The Docker image built from the `Dockerfile` must be capable of being used as either a `Server Peer`, `Client Peer` or, ideally, both.

The default mode of operation is as a `Server Peer` and the `ENTRYPOINT` in the `Dockerfile` should result in an [Server Peer](PeerConnectionTestSpecification.md) running when a new container is created. An example of how the [GitHub Action Script](../.github/workflows/peerconnection-test.yml) will create the container is:

`docker run ghcr.io/sipsorcery/sipsorcery-webrtc-echo`

If the image can also support operating as an `Client Peer` then a file called `client.sh` should exist in the root of the image file system. When a container is being used as in a client role the default `ENTRYPOINT` will be overruled by calling the `/client.sh` script. The script must take a single parameter of the URL of the `Server Peer`. An example of how the [GitHub Action Script](../.github/workflows/interop-peerconnection-echo.yml) will create a container to operate an an Echo Test client is:

`docker run --entrypoint "/client.sh" ghcr.io/sipsorcery/sipsorcery-webrtc-echo http://echo-server:8080/offer`