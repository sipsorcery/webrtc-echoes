cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_LINKER ld.lld)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")

project(libwebrtc-webrtc-echo VERSION 1.0)

add_executable(libwebrtc-webrtc-echo libwebrtc-webrtc-echo.cpp)
target_sources(libwebrtc-webrtc-echo PRIVATE
    fake_audio_capture_module.cc
    HttpSimpleServer.cpp 
    PcFactory.cpp 
    PcObserver.cpp)

add_definitions(-D_LIBCPP_ABI_UNSTABLE -D_LIBCPP_HAS_NO_VENDOR_AVAILABILITY_ANNOTATIONS -D_LIBCPP_DEBUG=0 -DWEBRTC_LINUX -DWEBRTC_POSIX -DUSE_AURA=1 -D_HAS_EXCEPTIONS=0 -D_SILENCE_ALL_CXX20_DEPRECATION_WARNINGS)

# SET(CMAKE_CXX_FLAGS "-fstack-protector -funwind-tables -fPIC -O0 -g2 -std=c++20")
set(CMAKE_CXX_FLAGS "-fstack-protector -funwind-tables -fPIC -frtti -std=c++20")

target_include_directories(libwebrtc-webrtc-echo PRIVATE
    /src/webrtc-checkout/src
    /src/webrtc-checkout/src/third_party/abseil-cpp)

SET(CMAKE_EXE_LINKER_FLAGS "-z noexecstack -z relro -z now -pie")

#link_directories( /src/webrtc-checkout/src/out/Default/obj)

target_link_libraries(libwebrtc-webrtc-echo
    -L/src/webrtc-checkout/src/out/Default/obj
    event
    webrtc # This will link to libwebrtc.a from the Builder image.
    dl
    pthread
    X11
    glib-2.0
    stdc++
    atomic)
