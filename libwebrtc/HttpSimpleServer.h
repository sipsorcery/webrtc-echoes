/******************************************************************************
* Filename: HttpSimpleServer.h
*
* Description:
* Simple HTTP server that's designed to only process a small number of 
* expected requests. No attempt is made to behave as a generic HTTP server.
*
* Author:
* Aaron Clauson (aaron@sipsorcery.com)
*
* History:
* 08 Mar 2021	Aaron Clauson	  Created, Dublin, Ireland.
*
* License: Public Domain (no warranty, use at own risk)
/******************************************************************************/

#ifndef __HTTP_SIMPLE_SERVER__
#define __HTTP_SIMPLE_SERVER__

#include "PcFactory.h"

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/thread.h>
#include <event2/util.h>

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

class HttpSimpleServer
{
public:
  HttpSimpleServer();
  ~HttpSimpleServer();
  void Init(const char * httpServerAddress, int httpServerPort, const char * offerPath);
  void Run();
  void Stop();
  
  static void SetPeerConnectionFactory(PcFactory* pcFactory);

private:
  event_base* _evtBase;
  evhttp* _httpSvr;
  event* _signalEvent;
  std::thread _httpSvrThread;
  bool _isDisposed;
  
  static PcFactory* _pcFactory;

  static void OnHttpRequest(struct evhttp_request* req, void* arg);
  static void OnSignal(evutil_socket_t sig, short events, void* user_data);
};

#endif
