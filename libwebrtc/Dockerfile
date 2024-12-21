FROM libwebrtc-builder:m132 as builder
FROM ubuntu:latest as appbuilder

# Install packages with the required runtime libraries.
RUN apt update && DEBIAN_FRONTEND="noninteractive" apt install -y \
  build-essential cmake libglib2.0-dev libx11-dev libevent-dev clang-18 lld-18

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100
RUN update-alternatives --install /usr/bin/ld ld /usr/bin/ld.lld-18 100

COPY --from=builder /src/webrtc-checkout/src /src/webrtc-checkout/src

WORKDIR /src/libwebrtc-webrtc-echo
COPY ["CMakeLists.txt", "fake_audio_capture_module.*", "HttpSimpleServer.*", "json.hpp", "libwebrtc-webrtc-echo.cpp", "PcFactory.*", "PcObserver.*", "./"]
WORKDIR /src/libwebrtc-webrtc-echo/build
#RUN cmake .. && make VERBOSE=1 && cp libwebrtc-webrtc-echo /

#FROM ubuntu:latest as final

# Install packages with the required runtime libraries.
#RUN apt update && DEBIAN_FRONTEND="noninteractive" apt install -y \
#   libevent-dev libx11-dev libglib2.0-dev libatomic1 --no-install-recommends

#COPY --from=appbuilder /libwebrtc-webrtc-echo /libwebrtc-webrtc-echo

#EXPOSE 8080
#ENTRYPOINT ["/libwebrtc-webrtc-echo"]