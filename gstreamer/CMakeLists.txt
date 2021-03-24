cmake_minimum_required(VERSION 3.10)
project(gstreamer-webrtc-echo VERSION 1.0)

add_executable(gstreamer-webrtc-echo gstreamer-webrtc-echo.c)
target_sources(gstreamer-webrtc-echo PRIVATE cJSON.c)

target_include_directories(gstreamer-webrtc-echo PRIVATE 
    /usr/local/include/gstreamer-1.0
    /usr/include/gstreamer-1.0
    /usr/include/glib-2.0
    /usr/lib/x86_64-linux-gnu/glib-2.0/include)

TARGET_LINK_LIBRARIES(gstreamer-webrtc-echo 
    glib-2.0
    gobject-2.0
    event
    gstreamer-full-1.0)
