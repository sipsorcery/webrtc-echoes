cmake_minimum_required(VERSION 3.5)
project(libwebrtc-webrtc-echo VERSION 1.0)

add_executable(libwebrtc-webrtc-echo libwebrtc-webrtc-echo.cpp)
target_sources(libwebrtc-webrtc-echo PRIVATE 
    HttpSimpleServer.cpp 
    fake_audio_capture_module.cc
    PcFactory.cpp 
    PcObserver.cpp)

add_definitions(-D_LIBCPP_ABI_UNSTABLE -D_LIBCPP_HAS_NO_VENDOR_AVAILABILITY_ANNOTATIONS -D_LIBCPP_DEBUG=0 -DWEBRTC_LINUX -DWEBRTC_POSIX)

SET(CMAKE_CXX_FLAGS "-fstack-protector -funwind-tables -fPIC -O0 -g2 -std=c++14")

target_include_directories(libwebrtc-webrtc-echo PRIVATE
    /src/webrtc-checkout/src
    /src/webrtc-checkout/src/third_party/abseil-cpp)

SET(CMAKE_EXE_LINKER_FLAGS "-z noexecstack -z relro -z now -pie")

link_directories(
    /src/webrtc-checkout/src/out/Default)

target_link_libraries(libwebrtc-webrtc-echo
    -L/src/webrtc-checkout/src/out/Default
    event       # Important that "event" precedes "webrtc-full" as the webrtc library contains duplicate, but older, symbols.
    webrtc-full
    dl
    pthread
    X11
    glib-2.0
    stdc++
    atomic)
