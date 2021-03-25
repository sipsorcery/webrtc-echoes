FROM canyan/janus-gateway:latest

COPY ["bin/Release/net5/publish/JanusWebRTCEcho", "janus-webrtc-echo"]
COPY ["entrypoint-dotnet.sh", "."]

EXPOSE 8080
ENTRYPOINT ["/entrypoint-dotnet.sh"]