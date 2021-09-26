FROM rust:latest AS build
WORKDIR /src
COPY ["webrtc-rs", ""]
RUN cd /src/server; cargo build
RUN cd /src/client; cargo build

FROM ubuntu:latest AS final
WORKDIR /app
EXPOSE 8080
#EXPOSE 49000-65535
COPY ["html", "../html/"]
COPY ["./webrtc-rs/client.sh", "/client.sh"]
RUN chmod +x /client.sh
COPY --from=build /src/client/target/debug/client .
COPY --from=build /src/server/target/debug/server .
ENTRYPOINT ["./server"]
