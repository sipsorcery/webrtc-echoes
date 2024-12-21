/******************************************************************************
* Filename: libwebrtc-webrtc-echo.cpp
*
* Description:
* Main program for a test program that creates a echo peer using Google's
* webrtc library, https://webrtc.googlesource.com/src/webrtc/.
*
* Dependencies:
* vcpkg install libevent:x64-windows
*
* Author:
* Aaron Clauson (aaron@sipsorcery.com)
*
* History:
* 08 Mar 2021	Aaron Clauson	  Created, Dublin, Ireland.
* 21 DEc 2024 Aaron Clauson   Updated for libwebrtc version m132.
*
* License: Public Domain (no warranty, use at own risk)
/******************************************************************************/

#include "HttpSimpleServer.h"
#include "PcFactory.h"

#include <api/create_peerconnection_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <rtc_base/logging.h>
#include <rtc_base/thread.h>

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <api/rtc_event_log/rtc_event_log_factory.h>

#define HTTP_SERVER_ADDRESS "0.0.0.0"
#define HTTP_SERVER_PORT 8080
#define HTTP_OFFER_URL "/offer"

int main()
{
  std::cout << "libwebrtc echo test server" << std::endl;

#ifdef _WIN32
  {
    /* If running on Windows need to initialise sockets. */
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
  }
#endif

  std::cout << "libevent version " << event_get_version() << "." << std::endl;

  rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_WARNING);

  {
    std::cout << "On main thread, thread ID " << std::this_thread::get_id() << std::endl;

    PcFactory pcFactory;

    HttpSimpleServer httpSvr;
    httpSvr.Init(HTTP_SERVER_ADDRESS, HTTP_SERVER_PORT, HTTP_OFFER_URL);
    HttpSimpleServer::SetPeerConnectionFactory(&pcFactory);

    httpSvr.Run();

    std::cout << "Stopping HTTP server..." << std::endl;

    httpSvr.Stop();
  }

#ifdef _WIN32
  WSACleanup();
#endif

  std::cout << "Exiting..." << std::endl;
}