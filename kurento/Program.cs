//-----------------------------------------------------------------------------
// Filename: Program.cs
//
// Description: This program acts as a gateway to a Kurento media server 
// instance to facilitate a WebRTC echo test. The program accepts an
// HTTP POST request with an SDP offer from a WebRTC peer and then 
// performs the necessary initialisation with the Kurento server to get it
// to act as the second peer.
//
// Remarks:
// A nightly build of the Kurento media server is available on dockerhub:
//
// docker run -it -p 8888:8888 kurento/kurento-media-server-dev
//
// Author(s):
// Aaron Clauson (aaron@sipsorcery.com)
//
// History:
// 16 Mar 2021	Aaron Clauson	Created, Dublin, Ireland.
//
// License: 
// BSD 3-Clause "New" or "Revised" License, see included LICENSE.md file.
//-----------------------------------------------------------------------------

using System;
using System.Net.WebSockets;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.VisualStudio.Threading;
using EmbedIO;
using Serilog;
using Serilog.Events;
using Serilog.Extensions.Logging;
using SIPSorcery.Net;
using StreamJsonRpc;

namespace KurentoEchoClient
{
    class Program
    {
        private const string DEFAULT_WEBSERVER_LISTEN_URL = "http://*:8080/";
        private const string KURENTO_JSONRPC_URL = "ws://localhost:8888/kurento";
        private const int KEEP_ALIVE_INTERVAL_MS = 240000;

        private static Microsoft.Extensions.Logging.ILogger logger = NullLogger.Instance;

        static async Task Main(string[] args)
        {
            Console.WriteLine("Kurento Echo Test Server starting...");

            logger = AddConsoleLogger(LogEventLevel.Verbose);

            // Start the web server.
            using (var server = CreateWebServer(DEFAULT_WEBSERVER_LISTEN_URL))
            {
                await server.RunAsync();

                Console.WriteLine("ctrl-c to exit.");
                var mre = new ManualResetEvent(false);
                Console.CancelKeyPress += (sender, eventArgs) =>
                {
                    // cancel the cancellation to allow the program to shutdown cleanly
                    eventArgs.Cancel = true;
                    mre.Set();
                };

                mre.WaitOne();
            }
        }

        private static WebServer CreateWebServer(string url)
        {
            var server = new WebServer(o => o
                    .WithUrlPrefix(url)
                    .WithMode(HttpListenerMode.EmbedIO))
                .WithCors("*", "*", "*")
                .WithAction("/offer", HttpVerbs.Post, OfferAsync);
            //.WithStaticFolder("/", "../../html", false);
            server.StateChanged += (s, e) => Console.WriteLine($"WebServer New State - {e.NewState}");

            return server;
        }

        private async static Task OfferAsync(IHttpContext context)
        {
            var offer = await context.GetRequestDataAsync<RTCSessionDescriptionInit>();
            logger.LogDebug($"SDP Offer={offer.sdp}");

            var jsonOptions = new JsonSerializerOptions();
            jsonOptions.Converters.Add(new System.Text.Json.Serialization.JsonStringEnumConverter());

            CancellationTokenSource cts = new CancellationTokenSource();

            var ws = new ClientWebSocket();
            await ws.ConnectAsync(new Uri(KURENTO_JSONRPC_URL), cts.Token);

            logger.LogDebug($"Successfully connected web socket client to {KURENTO_JSONRPC_URL}.");

            try
            {
                using (var jsonRpc = new JsonRpc(new WebSocketMessageHandler(ws)))
                {
                    jsonRpc.AddLocalRpcMethod("ping", new Action(() =>
                    {
                        logger.LogDebug($"Ping received");
                    }
                    ));

                    jsonRpc.AddLocalRpcMethod("onEvent", new Action<KurentoEvent>((evt) =>
                    {
                        logger.LogDebug($"Event received type={evt.type}, source={evt.data.source}");
                    }
                    ));

                    jsonRpc.StartListening();

                    // Check the server is there.
                    var pingResult = await jsonRpc.InvokeWithParameterObjectAsync<KurentoResult>(
                        KurentoMethodsEnum.ping.ToString(),
                        new { interval = KEEP_ALIVE_INTERVAL_MS },
                        cts.Token);
                    logger.LogDebug($"Ping result={pingResult.value}.");

                    // Create a media pipeline.
                    var createPipelineResult = await jsonRpc.InvokeWithParameterObjectAsync<KurentoResult>(
                        KurentoMethodsEnum.create.ToString(),
                        new { type = "MediaPipeline" },
                        cts.Token);
                    logger.LogDebug($"Create media pipeline result={createPipelineResult.value}, sessionID={createPipelineResult.sessionId}.");

                    var sessionID = createPipelineResult.sessionId;
                    var mediaPipeline = createPipelineResult.value;

                    // Create a WebRTC end point.
                    var createEndPointResult = await jsonRpc.InvokeWithParameterObjectAsync<KurentoResult>(
                        KurentoMethodsEnum.create.ToString(),
                        new
                        {
                            type = "WebRtcEndpoint",
                            constructorParams = new { mediaPipeline = mediaPipeline },
                            sessionId = sessionID
                        },
                        cts.Token);
                    logger.LogDebug($"Create WebRTC endpoint result={createEndPointResult.value}.");

                    var webRTCEndPointID = createEndPointResult.value;

                    // Connect the WebRTC end point to itself to create a loopback connection (no result for this operation).
                    await jsonRpc.InvokeWithParameterObjectAsync<KurentoResult>(
                        KurentoMethodsEnum.invoke.ToString(),
                        new
                        {
                            @object = webRTCEndPointID,
                            operation = "connect",
                            operationParams = new { sink = webRTCEndPointID },
                            sessionId = sessionID
                        },
                        cts.Token);

                    // Subscribe for events from the WebRTC end point.
                    var subscribeResult = await jsonRpc.InvokeWithParameterObjectAsync<KurentoResult>(
                        KurentoMethodsEnum.subscribe.ToString(),
                        new
                        {
                            @object = webRTCEndPointID,
                            type = "IceCandidateFound",
                            sessionId = sessionID
                        },
                        cts.Token);
                    logger.LogDebug($"Subscribe to WebRTC endpoint subscription ID={subscribeResult.value}.");

                    var subscriptionID = subscribeResult.value;

                    subscribeResult = await jsonRpc.InvokeWithParameterObjectAsync<KurentoResult>(
                        KurentoMethodsEnum.subscribe.ToString(),
                        new
                        {
                            @object = webRTCEndPointID,
                            type = "OnIceCandidate",
                            sessionId = sessionID
                        },
                        cts.Token);
                    logger.LogDebug($"Subscribe to WebRTC endpoint subscription ID={subscribeResult.value}.");

                    // Send SDP offer.
                    var processOfferResult = await jsonRpc.InvokeWithParameterObjectAsync<KurentoResult>(
                        KurentoMethodsEnum.invoke.ToString(),
                        new
                        {
                            @object = webRTCEndPointID,
                            operation = "processOffer",
                            operationParams = new { offer = offer.sdp },
                            sessionId = sessionID
                        },
                        cts.Token);

                    logger.LogDebug($"SDP answer={processOfferResult.value}.");

                    RTCSessionDescriptionInit answerInit = new RTCSessionDescriptionInit
                    {
                        type = RTCSdpType.answer,
                        sdp = processOfferResult.value
                    };

                    context.Response.ContentType = "application/json";
                    using (var responseStm = context.OpenResponseStream(false, false))
                    {
                        await JsonSerializer.SerializeAsync(responseStm, answerInit, jsonOptions);
                    }

                    // Tell Kurento to start ICE.
                    var gatherCandidatesResult = await jsonRpc.InvokeWithParameterObjectAsync<KurentoResult>(
                        KurentoMethodsEnum.invoke.ToString(),
                        new
                        {
                            @object = webRTCEndPointID,
                            operation = "gatherCandidates",
                            sessionId = sessionID
                        },
                        cts.Token);

                    logger.LogDebug($"Gather candidates result={gatherCandidatesResult.value}.");
                }
            }
            catch (RemoteInvocationException invokeExcp)
            {
                logger.LogError($"JSON RPC invoke exception, error code={invokeExcp.ErrorCode}, msg={invokeExcp.Message}.");
            }
        }

        private static Microsoft.Extensions.Logging.ILogger AddConsoleLogger(
            LogEventLevel logLevel = LogEventLevel.Debug)
        {
            var serilogLogger = new LoggerConfiguration()
                .Enrich.FromLogContext()
                .MinimumLevel.Is(logLevel)
                .WriteTo.Console()
                .CreateLogger();
            var factory = new SerilogLoggerFactory(serilogLogger);
            SIPSorcery.LogFactory.Set(factory);
            return factory.CreateLogger<Program>();
        }
    }
}
