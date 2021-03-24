FROM kurento/kurento-media-server-dev

COPY ["bin/Release/net5/publish/KurentoWebRTCEcho", "kurento-webrtc-echo"]
COPY ["entrypoint-dotnet.sh", "."]

EXPOSE 8080
ENTRYPOINT ["/entrypoint-dotnet.sh"]