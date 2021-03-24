/******************************************************************************
* Filename: gst-webrtc-echo.cpp
*
* Description:
* This file contains a C console application that acts as a WebRTC Echo Test
* Server using the GStreamer webrtcbin plugin.
*
* webrtcbin plugin reference:
* https://gstreamer.freedesktop.org/documentation/webrtc/index.html?gi-language=c
*
* Ubuntu:
*  - apt install libevent (or libevent-dev on focal)
*  - clone cJSON and build with cmake
*  - apt install libglib2.0
*
* Remarks:
* To find the properties and signals available for the webrtcbin plugin see:
* https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad/-/blob/master/ext/webrtc/gstwebrtcbin.c#L6489
* 
* Author:
* Aaron Clauson (aaron@sipsorcery.com)
*
* History:
* 26 Feb 2021	Aaron Clauson	  Created, Dublin, Ireland.
* 02 Mar 2021 Aaron Clauson   Linuxified (WSL).
*
* License: Public Domain (no warranty, use at own risk)
/******************************************************************************/

#define GST_USE_UNSTABLE_API

#include "cJSON.h"
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>
#include <gst/webrtc/dtlstransport.h>

#include <stdio.h>
#include <string.h>

#define HTTP_SERVER_ADDRESS "0.0.0.0"
#define HTTP_SERVER_PORT 8080
#define HTTP_OFFER_URL "/offer"
#define RTP_CAPS_VP8 "application/x-rtp,media=video,encoding-name=VP8,payload=96"
//#define RTP_CAPS_H264 "application/x-rtp,media=video,encoding-name=H264,payload=104"

static void on_http_request_cb(struct evhttp_request* req, void* arg);
static GstElement* create_webrtc();
static void on_negotiation_needed (GstElement* element, gpointer user_data);
static void send_ice_candidate_message (GstElement* webrtc G_GNUC_UNUSED, guint mlineindex, gchar* candidate, gpointer user_data G_GNUC_UNUSED);
static void on_new_transceiver (GstElement* object, GstWebRTCRTPTransceiver* candidate, gpointer udata);
static void on_ice_gathering_state_notify (GstElement* webrtcbin, GParamSpec* pspec, gpointer user_data);
static void on_ice_connection_state_notify (GstElement* webrtcbin, GParamSpec* pspec, gpointer user_data);
static void on_connection_state_notify (GstElement* webrtcbin, GParamSpec* pspec, gpointer user_data);
static void on_offer_set (GstPromise* promise, gpointer user_data);
static void on_answer_created (GstPromise* promise, gpointer user_data);
static void set_offer(GstElement* webrtc, const gchar* sdp_offer_str);
static gboolean bus_call (GstBus* bus, GstMessage* msg, gpointer data);

int main(int argc, char* argv[])
{
  GMainLoop* gst_main_loop;
  GThread* main_loop_thread;
  struct event_base* base = NULL;
  struct evhttp* httpSvr = NULL;
  int res = 0;

#ifdef _WIN32
  {
    /* If running on Windows need to initialise sockets. */
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
  }
#endif

  /* Initialise GStreamer. */
  gst_init (&argc, &argv);

  gst_main_loop = g_main_loop_new(NULL, FALSE);
  main_loop_thread = g_thread_new("main_loop", (GThreadFunc)g_main_loop_run, gst_main_loop);
  if (main_loop_thread == NULL) {
    fprintf(stderr, "Couldn't create a main loop thread.\n");
    return -1;
  }

  /* Initialise libevent HTTP server. */
  base = event_base_new();
  if (!base) {
    fprintf(stderr, "Couldn't create an event_base: exiting.\n");
    return -1;
  }

  httpSvr = evhttp_new(base);
  if (!httpSvr) {
    fprintf(stderr, "couldn't create evhttp. Exiting.\n");
    return -1;
  }

  res = evhttp_bind_socket(httpSvr, HTTP_SERVER_ADDRESS, HTTP_SERVER_PORT);
  if (res != 0) {
    fprintf(stderr, "Failed to start HTTP server on %s:%d.\n", HTTP_SERVER_ADDRESS, HTTP_SERVER_PORT);
    return res;
  }

  evhttp_set_allowed_methods(httpSvr,
    EVHTTP_REQ_GET |
    EVHTTP_REQ_POST |
    EVHTTP_REQ_OPTIONS);

  printf("Waiting for SDP offer on http://%s:%d%s...\n", HTTP_SERVER_ADDRESS, HTTP_SERVER_PORT, HTTP_OFFER_URL);

  res = evhttp_set_cb(httpSvr, HTTP_OFFER_URL, on_http_request_cb, NULL);

  event_base_dispatch(base);

  g_main_loop_unref (gst_main_loop);

  evhttp_free(httpSvr);

#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}

/**
* The handler function for an incoming HTTP request. This is the start of the 
* handling for any WebRTC peer that wishes to establish a connection. The incoming
* request MUST have an SDP offer in its body.
* @param[in] req: the HTTP request received from the remote client.
* @param[in] arg: not used.
*/
static void on_http_request_cb(struct evhttp_request* req, void* arg)
{
  const char* uri = evhttp_request_get_uri(req);
  struct evbuffer* http_req_body;
  size_t http_req_body_len;
  unsigned char* http_req_buffer;
  const cJSON* sdp_init_offer_json = NULL;
  const cJSON* sdp_json = NULL;
  cJSON* sdp_json_answer;
  cJSON* sdp_json_answer_type;
  cJSON* sdp_json_answer_sdp;
  char* json_response;
  struct evbuffer* resp_buffer;
  size_t response_length;
  int resp_lock = 0;
  GstElement* webrtcbin;
  GstPromise* create_answer_promise;
  const GstStructure* answer_reply;
  GstWebRTCSessionDescription* answer;
  gchar* answer_sdp_text;

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

    if (!evbuffer_enable_locking(resp_buffer, NULL)) {
      fprintf(stderr, "Failed to create a lock for the HTTP response buffer.\n");
    }

    evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");

    http_req_body = evhttp_request_get_input_buffer(req);
    http_req_body_len = evbuffer_get_length(http_req_body);

    if (http_req_body_len > 0) {
      http_req_buffer = calloc(http_req_body_len + 1, sizeof(char));

      evbuffer_copyout(http_req_body, http_req_buffer, http_req_body_len);

      printf("HTTP request body length %zu.\n", http_req_body_len);

      //printf("Body: %s\n", sdp_buffer);

      sdp_init_offer_json = cJSON_Parse(http_req_buffer);

      printf("sdp offer: %s\n", cJSON_Print(sdp_init_offer_json));

      sdp_json = cJSON_GetObjectItemCaseSensitive(sdp_init_offer_json, "sdp");

      if (cJSON_IsString(sdp_json) && (sdp_json->valuestring != NULL)) {

        webrtcbin = create_webrtc();

        if (webrtcbin != NULL) {

          set_offer(webrtcbin, sdp_json->valuestring);

          create_answer_promise = gst_promise_new();
          g_signal_emit_by_name(webrtcbin, "create-answer", NULL, create_answer_promise);

          if (gst_promise_wait(create_answer_promise) == GST_PROMISE_RESULT_REPLIED) {

            printf("create-answer promise wait over.\n");

            answer_reply = gst_promise_get_reply (create_answer_promise);

            gst_structure_get (answer_reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
            gst_promise_unref (create_answer_promise);

            if (answer != NULL) {

              answer_sdp_text = gst_sdp_message_as_text(answer->sdp);
              gst_webrtc_session_description_free (answer);

              sdp_json_answer = cJSON_CreateObject();
              sdp_json_answer_type = cJSON_CreateString("answer");
              cJSON_AddItemToObject(sdp_json_answer, "type", sdp_json_answer_type);
              sdp_json_answer_sdp = cJSON_CreateString(answer_sdp_text);
              cJSON_AddItemToObject(sdp_json_answer, "sdp", sdp_json_answer_sdp);
              json_response = cJSON_Print(sdp_json_answer);

              printf("Return SDP answer to client: %s.\n", json_response);

              response_length = strlen(json_response);
              evhttp_add_header(req->output_headers, "Content-type", "application/json");
              evbuffer_add(resp_buffer, json_response, response_length);
              evhttp_send_reply(req, 200, "OK", resp_buffer);
            }
            else {
              evbuffer_add_printf(resp_buffer, "Failed to get webrtc SDP answer.");
              evhttp_send_reply(req, 501, "Internal Server Error", resp_buffer);
            }
          }
          else {
            evbuffer_add_printf(resp_buffer, "Timed out waiting for webrtc SDP answer.");
            evhttp_send_reply(req, 501, "Internal Server Error", resp_buffer);
          }
        }
        else {
          evbuffer_add_printf(resp_buffer, "Failed to initialise webrtc peer connection.");
          evhttp_send_reply(req, 501, "Internal Server Error", resp_buffer);
        }
      }
      else {
        evbuffer_add_printf(resp_buffer, "Could not parse SDP offer.");
        evhttp_send_reply(req, 400, "Bad Request", resp_buffer);
      }
    }
    else {
      evbuffer_add_printf(resp_buffer, "Request was missing the SDP offer.");
      evhttp_send_reply(req, 400, "Bad Request", resp_buffer);
    }
  }
}

/**
* Attempts to create the gstreamer WebRTC pipeline. Signal handlers
* for important events are attached to the WebRTC object and will be
* responsible for progressing the WebRTC connection subsequent to its
* creation.
* @@Returns a new WebRTC object.
*/
static GstElement* create_webrtc()
{
  GstElement* pipeline, * webrtcbin;
  GstBus* bus;
  GstStateChangeReturn ret;
  GError* error = NULL;

  pipeline =
     gst_parse_launch ("webrtcbin bundle-policy=max-bundle name=sendonly "
       "videotestsrc is-live=true pattern=ball ! videoconvert ! queue ! vp8enc deadline=1 ! rtpvp8pay ! "
       "queue ! " RTP_CAPS_VP8 " ! sendonly. "
       , &error);

  //pipeline =
  //  gst_parse_launch ("webrtcbin bundle-policy=max-bundle name=sendonly "
  //    "videotestsrc is-live=true pattern=ball ! videoconvert ! queue ! openh264enc ! rtph264pay ! "
  //    "queue ! " RTP_CAPS_H264 " ! sendonly. "
  //    , &error);

  if (error) {
    gst_printerr ("Failed to parse launch: %s\n", error->message);
    g_error_free (error);
    return NULL;
  }

  /*pipeline = gst_pipeline_new ("echo-test-pipeline");
  webrtcbin = gst_element_factory_make ("webrtcbin", "webrtcx");

  if (!pipeline || !webrtcbin) {
    g_printerr("Unable to initialise echo test pipeline.\n");
  }

  gst_bin_add_many (GST_BIN (pipeline), webrtcbin, NULL);
  if (gst_element_link (webrtcbin, webrtcbin) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
  }*/

  webrtcbin = gst_bin_get_by_name (GST_BIN (pipeline), "sendonly");
  g_assert_nonnull (webrtcbin);

  g_signal_connect (webrtcbin, "on-negotiation-needed", G_CALLBACK (on_negotiation_needed), NULL);
  g_signal_connect (webrtcbin, "on-ice-candidate", G_CALLBACK (send_ice_candidate_message), NULL);
  g_signal_connect (webrtcbin, "on-new-transceiver", G_CALLBACK (on_new_transceiver), NULL);
  g_signal_connect (webrtcbin, "notify::on-new-transceiver", G_CALLBACK (on_new_transceiver), NULL);
  g_signal_connect (webrtcbin, "notify::ice-gathering-state", G_CALLBACK (on_ice_gathering_state_notify), NULL);
  g_signal_connect (webrtcbin, "notify::ice-connection-state", G_CALLBACK (on_ice_connection_state_notify), NULL);
  g_signal_connect (webrtcbin, "notify::connection-state", G_CALLBACK (on_connection_state_notify), NULL);
  
  /* Start playing */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(pipeline);
    return NULL;
  }

  bus = gst_element_get_bus (pipeline);
  gst_bus_add_watch (bus, bus_call, NULL);
  gst_object_unref (bus);

  return webrtcbin;
}

/**
* 
* @param[in] webrtc: the HTTP request received from the remote client.
* @param[in] sdp_offer_str: not used.
*/
static void set_offer(GstElement* webrtc, const gchar* sdp_offer_str)
{
  GstWebRTCSessionDescription* offer = NULL;
  GstPromise* promise;
  GstSDPMessage* sdp;
  int ret;

  printf("set_offer.\n");

  ret = gst_sdp_message_new (&sdp);
  g_assert_cmphex (ret, == , GST_SDP_OK);
  ret = gst_sdp_message_parse_buffer (sdp_offer_str, (guint)strlen (sdp_offer_str), sdp);
  g_assert_cmphex (ret, == , GST_SDP_OK);

  offer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_OFFER, sdp);
  g_assert_nonnull (offer);

  /* Set remote description on our pipeline */
  promise = gst_promise_new_with_change_func (on_offer_set, webrtc, NULL);
  g_signal_emit_by_name (webrtc, "set-remote-description", offer, promise);
}

static void on_offer_set (GstPromise* promise, gpointer webrtc)
{
  const GstStructure* reply;
  const GstPromise* answer_promise;

  printf("on_offer_set.\n");

  reply = gst_promise_get_reply (promise);
  gst_promise_unref (promise);

  answer_promise = gst_promise_new_with_change_func (on_answer_created, webrtc, NULL);
  g_signal_emit_by_name (webrtc, "create-answer", NULL, answer_promise);
}

/* Answer created by our pipeline, to be sent to the peer */
static void on_answer_created (GstPromise* promise, gpointer webrtc)
{
  GstWebRTCSessionDescription* answer;
  const GstStructure* reply;
  const gchar* answer_sdp_text = NULL;
  GstPromise* set_local_promise;

  printf("on_answer_created.\n");

  g_assert_cmphex (gst_promise_wait (promise), == , GST_PROMISE_RESULT_REPLIED);
  reply = gst_promise_get_reply (promise);
  gst_structure_get (reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
  gst_promise_unref (promise);

  set_local_promise = gst_promise_new ();
  g_signal_emit_by_name (webrtc, "set-local-description", answer, set_local_promise);
  gst_promise_interrupt (set_local_promise);
  gst_promise_unref (set_local_promise);

  //answer_sdp_text = gst_sdp_message_as_text(answer->sdp);

  //printf("Answer SDP: %s.\n", answer_sdp_text);

  //_sdp_answer_str = answer_sdp_text;

  gst_webrtc_session_description_free (answer);
}

static void on_negotiation_needed (GstElement* element, gpointer user_data)
{
  gchar* name;
  GstWebRTCSignalingState signalingState = 0;
  GstWebRTCSessionDescription* remoteDescription = NULL;

  printf("on_negotiation_needed\n");

  g_object_get (G_OBJECT (element), "name", &name, NULL);
  g_print ("The name of the element is '%s'.\n", name);
  g_free (name);

  g_object_get (G_OBJECT (element), "signaling-state", &signalingState, NULL);
  g_print ("The signaling state of the element is '%d'.\n", signalingState);

  g_object_get (G_OBJECT (element), "remote-description", &remoteDescription, NULL);
  if (remoteDescription == NULL) {
    g_print("Remote description is not set.\n");
  }
  else {
    g_print("Remote description is set.\n");
  }
}

static void send_ice_candidate_message (GstElement* webrtc G_GNUC_UNUSED, guint mlineindex,
  gchar* candidate, gpointer user_data G_GNUC_UNUSED)
{
  //printf("send_ice_candidate_message\n");
}

static void on_new_transceiver (GstElement* object, GstWebRTCRTPTransceiver* candidate, gpointer udata)
{
  g_print("on_new_transceiver.\n");
}

static void on_ice_gathering_state_notify (GstElement* webrtcbin, GParamSpec* pspec, gpointer user_data)
{
  GstWebRTCICEGatheringState ice_gathering_state = 0;
  g_object_get (G_OBJECT (webrtcbin), "ice-gathering-state", &ice_gathering_state, NULL);
  g_print ("on_ice_gathering_state_notify '%d'.\n", ice_gathering_state);
}

static void on_ice_connection_state_notify (GstElement* webrtcbin, GParamSpec* pspec, gpointer user_data)
{
  GstWebRTCICEConnectionState ice_connection_state = 0;
  g_object_get (G_OBJECT (webrtcbin), "ice-connection-state", &ice_connection_state, NULL);
  g_print ("on_ice_connection_state_notify '%d'.\n", ice_connection_state);
}

static void on_connection_state_notify (GstElement* webrtcbin, GParamSpec* pspec, gpointer user_data)
{
  GstWebRTCPeerConnectionState connection_state = 0;
  g_object_get (G_OBJECT (webrtcbin), "connection-state", &connection_state, NULL);
  g_print ("on_connection_state_notify '%d'.\n", connection_state);

  if (connection_state == GST_WEBRTC_PEER_CONNECTION_STATE_FAILED) {
    g_print("Peer connections failed, shutting down pipeline.\n");
    gst_element_set_state (webrtcbin, GST_STATE_NULL);
    gst_object_unref (webrtcbin);
  }
}

static gboolean bus_call (GstBus* bus, GstMessage* msg, gpointer data)
{
  //GMainLoop* loop = (GMainLoop*)data;

  //g_print("bus_call %d.\n", GST_MESSAGE_TYPE (msg));

  switch (GST_MESSAGE_TYPE (msg)) {

  case GST_MESSAGE_EOS:
    g_print ("End of stream\n");
    //g_main_loop_quit (loop);
    break;

  case GST_MESSAGE_ERROR: {
    gchar* debug;
    GError* error;

    gst_message_parse_error (msg, &error, &debug);
    g_free (debug);

    g_printerr ("Error: %s\n", error->message);
    g_error_free (error);

    //g_main_loop_quit (loop);
    break;
  }
  default:
    break;
  }

  return TRUE;
}
