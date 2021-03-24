/******************************************************************************
* Filename: HttpSimpleServer.cpp
*
* Description: See header file.
*
* Author:
* Aaron Clauson (aaron@sipsorcery.com)
*
* History:
* 08 Mar 2021	Aaron Clauson	  Created, Dublin, Ireland.
*
* License: Public Domain (no warranty, use at own risk)
/******************************************************************************/

#include "HttpSimpleServer.h"

#include <signal.h>

PcFactory* HttpSimpleServer::_pcFactory = nullptr;

HttpSimpleServer::HttpSimpleServer() :
  _isDisposed(false)
{
#ifdef _WIN32
  evthread_use_windows_threads();
#else
  //evthread_use_pthreads();
#endif

  /* Initialise libevent HTTP server. */
  _evtBase = event_base_new();
  if (!_evtBase) {
    throw std::runtime_error("HttpSimpleServer couldn't create an event_base instance.");
  }

  _httpSvr = evhttp_new(_evtBase);
  if (!_httpSvr) {
    throw std::runtime_error("HttpSimpleServer couldn't create an evhttp instance.");
  }

  _signalEvent = evsignal_new(_evtBase, SIGINT, HttpSimpleServer::OnSignal, (void*)_evtBase);
  if (!_signalEvent) {
    throw std::runtime_error("HttpSimpleServer couldn't create an event instance with evsignal_new.");
  }

  int addResult = event_add(_signalEvent, NULL);
  if (addResult < 0) {
    std::cerr << "Failed to add signal event handler." << std::endl;
  }
}

HttpSimpleServer::~HttpSimpleServer() {
  if (!_isDisposed) {
    _isDisposed = true;
    event_base_loopexit(_evtBase, nullptr);
    evhttp_free(_httpSvr);
    event_free(_signalEvent);
    event_base_free(_evtBase);
  }
}

void HttpSimpleServer::Init(const char* httpServerAddress, int httpServerPort, const char* offerPath) {

  int res = evhttp_bind_socket(_httpSvr, httpServerAddress, httpServerPort);
  if (res != 0) {
    throw std::runtime_error("HttpSimpleServer failed to start HTTP server on " +
      std::string(httpServerAddress) + ":" + std::to_string(httpServerPort) + ".");
  }

  evhttp_set_allowed_methods(_httpSvr,
    EVHTTP_REQ_GET |
    EVHTTP_REQ_POST |
    EVHTTP_REQ_OPTIONS);

  std::cout << "Waiting for SDP offer on http://"
    + std::string(httpServerAddress) + ":" + std::to_string(httpServerPort) + offerPath << std::endl;

  res = evhttp_set_cb(_httpSvr, offerPath, HttpSimpleServer::OnHttpRequest, NULL);
  if (res != 0) {
    throw std::runtime_error("HttpSimpleServer failed to set request callback.");
  }
}

void HttpSimpleServer::Run() {
  event_base_dispatch(_evtBase);
}

void HttpSimpleServer::Stop() {
  this->~HttpSimpleServer();
}

void HttpSimpleServer::SetPeerConnectionFactory(PcFactory* pcFactory) {
  _pcFactory = pcFactory;
}

/**
* The handler function for an incoming HTTP request. This is the start of the
* handling for any WebRTC peer that wishes to establish a connection. The incoming
* request MUST have an SDP offer in its body.
* @param[in] req: the HTTP request received from the remote client.
* @param[in] arg: not used.
*/
void HttpSimpleServer::OnHttpRequest(struct evhttp_request* req, void* arg)
{
  const char* uri = evhttp_request_get_uri(req);
  evbuffer* http_req_body = nullptr;
  size_t http_req_body_len{ 0 };
  char* http_req_buffer = nullptr;
  struct evbuffer* resp_buffer;
  int resp_lock = 0;

  printf("Received HTTP request for %s.\n", uri);

  if (req->type == EVHTTP_REQ_OPTIONS) {
    evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");
    evhttp_add_header(req->output_headers, "Access-Control-Allow-Methods", "POST");
    evhttp_add_header(req->output_headers, "Access-Control-Allow-Headers", "content-type");
    evhttp_send_reply(req, 200, "OK", NULL);
  }
  else {

    resp_buffer = evbuffer_new();
    if (!resp_buffer) {
      fprintf(stderr, "Failed to create HTTP response buffer.\n");
    }

    evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");

    http_req_body = evhttp_request_get_input_buffer(req);
    http_req_body_len = evbuffer_get_length(http_req_body);

    if (http_req_body_len > 0) {
      http_req_buffer = static_cast<char*>(calloc(http_req_body_len, sizeof(char)));

      evbuffer_copyout(http_req_body, http_req_buffer, http_req_body_len);

      printf("HTTP request body length %zu.\n", http_req_body_len);

      std::string offerJson(http_req_buffer, http_req_body_len);
      std::cout << offerJson << std::endl;

      if (_pcFactory != nullptr) {
        std::string answer = _pcFactory->CreatePeerConnection(http_req_buffer, http_req_body_len);
        evhttp_add_header(req->output_headers, "Content-type", "application/json");
        evbuffer_add_printf(resp_buffer, answer.c_str());
        evhttp_send_reply(req, 200, "OK", resp_buffer);
      }
      else {
        evbuffer_add_printf(resp_buffer, "No handler");
        evhttp_send_reply(req, 400, "Bad Request", resp_buffer);
      }
    }
    else {
      evbuffer_add_printf(resp_buffer, "Request was missing the SDP offer.");
      evhttp_send_reply(req, 400, "Bad Request", resp_buffer);
    }
  }
}

void HttpSimpleServer::OnSignal(evutil_socket_t sig, short events, void* user_data)
{
  event_base* base = static_cast<event_base*>(user_data);

  std::cout << "Caught an interrupt signal; calling loop exit." << std::endl;

  event_base_loopexit(base, nullptr);
}
