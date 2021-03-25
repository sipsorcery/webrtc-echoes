cmake_minimum_required(VERSION 3.7)
project(webrtc-libdatachannel
	VERSION 0.1.0
	LANGUAGES CXX)
set(PROJECT_DESCRIPTION "WebRTC Data Channels Library")

# Dependencies

option(NO_WEBSOCKET "Disable WebSocket support for libdatachannel" ON)
add_subdirectory(deps/libdatachannel EXCLUDE_FROM_ALL)
add_subdirectory(deps/cpp-httplib EXCLUDE_FROM_ALL)

# Client

add_executable(webrtc-libdatachannel-client client.cpp)
set_target_properties(webrtc-libdatachannel-client PROPERTIES
	VERSION ${PROJECT_VERSION}
        CXX_STANDARD 17
	OUTPUT_NAME client)

target_link_libraries(webrtc-libdatachannel-client datachannel-static httplib nlohmann_json)

# Server

add_executable(webrtc-libdatachannel-server server.cpp)
set_target_properties(webrtc-libdatachannel-server PROPERTIES
	VERSION ${PROJECT_VERSION}
        CXX_STANDARD 17
	OUTPUT_NAME server)

target_link_libraries(webrtc-libdatachannel-server datachannel-static httplib nlohmann_json)

