//-----------------------------------------------------------------------------
// Filename: Program.cs
//
// Description: This program acts as a gateway to a Janus media server 
// instance to facilitate a WebRTC echo test. The program accepts an
// HTTP POST request with an SDP offer from a WebRTC peer and then 
// performs the necessary initialisation with the Janus server to get it
// to act as the second peer.
//
// Remarks:
// docker run -it --init -p 8088:8088 canyan/janus-gateway:latest
//
// Author(s):
// Aaron Clauson (aaron@sipsorcery.com)
//
// History:
// 25 Mar 2021	Aaron Clauson	Created, Dublin, Ireland.
//
// License: 
// BSD 3-Clause "New" or "Revised" License, see included LICENSE.md file.
//-----------------------------------------------------------------------------

using System;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using EmbedIO;
using Serilog;
using Serilog.Events;
using Serilog.Extensions.Logging;
using SIPSorcery.Net;

namespace JanusEchoClient
{
    class Program
    {
        private const string DEFAULT_WEBSERVER_LISTEN_URL = "http://*:8080/";
        private const string JANUS_REST_URL = "http://localhost:8088/janus/";
        private const int MAX_TEST_DURATION_SECONDS = 30;

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
            cts.CancelAfter(MAX_TEST_DURATION_SECONDS * 1000);

            var janusClient = new JanusRestClient(JANUS_REST_URL, logger, cts.Token);

            var startSessionResult = await janusClient.StartSession();

            if (!startSessionResult.Success)
            {
                throw HttpException.InternalServerError("Failed to create Janus session.");
            }
            else
            {
                var waitForAnswer = new TaskCompletionSource<string>();

                janusClient.OnJanusEvent += (resp) =>
                {
                    if (resp.jsep != null)
                    {
                        logger.LogDebug($"janus get event jsep={resp.jsep.type}.");
                        logger.LogDebug($"janus SDP Answer: {resp.jsep.sdp}");

                        waitForAnswer.SetResult(resp.jsep.sdp);
                    }
                };

                var pluginResult = await janusClient.StartEcho(offer.sdp);
                if (!pluginResult.Success)
                {
                    await janusClient.DestroySession();
                    waitForAnswer.SetCanceled();
                    cts.Cancel();
                    throw HttpException.InternalServerError("Failed to create Janus Echo Test Plugin instance.");
                }
                else
                {
                    using (cts.Token.Register(() =>
                    {
                        // This callback will be executed if the token is cancelled.
                        waitForAnswer.TrySetCanceled();
                    }))
                    {
                        // At this point we're waiting for a response on the Janus long poll thread that
                        // contains the SDP answer to send back to the client.
                        try
                        {
                            var answer = await waitForAnswer.Task;
                            logger.LogDebug("SDP answer ready, sending to client.");

                            if (answer != null)
                            {
                                var answerInit = new RTCSessionDescriptionInit
                                {
                                    type = RTCSdpType.answer,
                                    sdp = answer
                                };

                                context.Response.ContentType = "application/json";
                                using (var responseStm = context.OpenResponseStream(false, false))
                                {
                                    await JsonSerializer.SerializeAsync(responseStm, answerInit, jsonOptions);
                                }
                            }
                        }
                        catch (TaskCanceledException)
                        {
                            await janusClient.DestroySession();
                            throw HttpException.InternalServerError("Janus operation timed out waiting for Echo Test plugin SDP answer.");
                        }
                    }
                }
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
