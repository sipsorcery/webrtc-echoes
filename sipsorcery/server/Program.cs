using System;
using System.Collections.Generic;
using System.Net;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using EmbedIO;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using Serilog;
using Serilog.Events;
using Serilog.Extensions.Logging;
using SIPSorcery.Net;
using SIPSorceryMedia.Abstractions;

namespace webrtc_echo
{
    class Program
    {
        private const string DEFAULT_WEBSERVER_LISTEN_URL = "http://*:8080/";

        private static Microsoft.Extensions.Logging.ILogger logger = NullLogger.Instance;

        private static List<IPAddress> _icePresets = new List<IPAddress>();

        static void Main(string[] args)
        {
            var url = DEFAULT_WEBSERVER_LISTEN_URL;
         
            // Apply any command line options
            if (args.Length > 0)
            {
                url = args[0];
                for(int i=1; i<args.Length; i++)
                {
                    if(IPAddress.TryParse(args[i], out var addr))
                    {
                        _icePresets.Add(addr);
                        Console.WriteLine($"ICE candidate preset address {addr} added.");
                    }
                }
            }

            logger = AddConsoleLogger(LogEventLevel.Information);

            // Start the web server.
            using (var server = CreateWebServer(url))
            {
                server.RunAsync();

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
                .WithAction("/offer", HttpVerbs.Post, Offer)
                .WithStaticFolder("/", "../../html", false);
            server.StateChanged += (s, e) => Console.WriteLine($"WebServer New State - {e.NewState}");

            return server;
        }

        private async static Task Offer(IHttpContext context)
        {
            var offer = await context.GetRequestDataAsync<RTCSessionDescriptionInit>();

            var jsonOptions = new JsonSerializerOptions();
            jsonOptions.Converters.Add(new System.Text.Json.Serialization.JsonStringEnumConverter());

            var echoServer = new WebRTCEchoServer(_icePresets);
            var answer = await echoServer.GotOffer(offer);

            if (answer != null)
            {
                context.Response.ContentType = "application/json";
                using (var responseStm = context.OpenResponseStream(false, false))
                {
                    await JsonSerializer.SerializeAsync(responseStm, answer, jsonOptions);
                }
            }
        }

        private static Microsoft.Extensions.Logging.ILogger AddConsoleLogger(
            Serilog.Events.LogEventLevel logLevel = Serilog.Events.LogEventLevel.Debug)
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

    public class WebRTCEchoServer
    {
        private const int VP8_PAYLOAD_ID = 96;

        private static Microsoft.Extensions.Logging.ILogger logger = NullLogger.Instance;

        private List<IPAddress> _presetIceAddresses;

        public WebRTCEchoServer(List<IPAddress> presetAddresses)
        {
            logger = SIPSorcery.LogFactory.CreateLogger<WebRTCEchoServer>();
            _presetIceAddresses = presetAddresses;
        }

        public async Task<RTCSessionDescriptionInit> GotOffer(RTCSessionDescriptionInit offer)
        {
            logger.LogTrace($"SDP offer received.");
            logger.LogTrace(offer.sdp);

            var pc = new RTCPeerConnection();

            if (_presetIceAddresses != null)
            {
                foreach (var addr in _presetIceAddresses)
                {
                    var rtpPort = pc.GetRtpChannel().RTPPort;
                    var publicIPv4Candidate = new RTCIceCandidate(RTCIceProtocol.udp, addr, (ushort)rtpPort, RTCIceCandidateType.host);
                    pc.addLocalIceCandidate(publicIPv4Candidate);
                }
            }

            MediaStreamTrack audioTrack = new MediaStreamTrack(SDPWellKnownMediaFormatsEnum.PCMU);
            pc.addTrack(audioTrack);
            MediaStreamTrack videoTrack = new MediaStreamTrack(new VideoFormat(VideoCodecsEnum.VP8, VP8_PAYLOAD_ID));
            pc.addTrack(videoTrack);

            pc.OnRtpPacketReceived += (IPEndPoint rep, SDPMediaTypesEnum media, RTPPacket rtpPkt) =>
            {
                pc.SendRtpRaw(media, rtpPkt.Payload, rtpPkt.Header.Timestamp, rtpPkt.Header.MarkerBit, rtpPkt.Header.PayloadType);
            };

            pc.OnTimeout += (mediaType) => logger.LogWarning($"Timeout for {mediaType}.");
            pc.oniceconnectionstatechange += (state) => logger.LogInformation($"ICE connection state changed to {state}.");
            pc.onsignalingstatechange += () => logger.LogInformation($"Signaling state changed to {pc.signalingState}.");
            pc.onconnectionstatechange += (state) =>
            {
                logger.LogInformation($"Peer connection state changed to {state}.");
                if (state == RTCPeerConnectionState.failed)
                {
                    pc.Close("ice failure");
                }
            };

            var setResult = pc.setRemoteDescription(offer);
            if (setResult == SetDescriptionResultEnum.OK)
            {
                var offerSdp = pc.createOffer(null);
                await pc.setLocalDescription(offerSdp);

                var answer = pc.createAnswer(null);

                logger.LogTrace($"SDP answer created.");
                logger.LogTrace(answer.sdp);

                return answer;
            }
            else
            {
                logger.LogWarning($"Failed to set remote description {setResult}.");
                return null;
            }
        }
    }
}
