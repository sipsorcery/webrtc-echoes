using System;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using Serilog;
using Serilog.Events;
using Serilog.Extensions.Logging;
using SIPSorcery.Media;
using SIPSorcery.Net;
using SIPSorceryMedia.Abstractions;

namespace webrtc_echo
{
    class Program
    {
        private const string DEFAULT_ECHO_SERVER_URL = "http://localhost:8080/offer";

        private static Microsoft.Extensions.Logging.ILogger logger = NullLogger.Instance;

        static async Task Main(string[] args)
        {
            Console.WriteLine("Starting webrtc echo test client.");

            logger = AddConsoleLogger(LogEventLevel.Debug);

            var pc = CreatePeerConnection();
            var offer = pc.createOffer(null);
            await pc.setLocalDescription(offer);

            var httpClient = new HttpClient();
            var content = new StringContent(offer.toJSON(), Encoding.UTF8, "application/json");
            var response = await httpClient.PostAsync(DEFAULT_ECHO_SERVER_URL, content);
            var answerStr = await response.Content.ReadAsStringAsync();

            if (RTCSessionDescriptionInit.TryParse(answerStr, out var answerInit))
            {
                var setAnswerResult = pc.setRemoteDescription(answerInit);
                if (setAnswerResult != SetDescriptionResultEnum.OK)
                {
                    Console.WriteLine($"Set remote description failed {setAnswerResult}.");
                }
            }
            else
            {
                Console.WriteLine("Failed to parse SDP answer from echo server.");
            }

            Console.WriteLine("ctrl-c to exit.");
            var mre = new ManualResetEvent(false);
            Console.CancelKeyPress += (sender, eventArgs) =>
            {
                    // cancel the cancellation to allow the program to shutdown cleanly
                    eventArgs.Cancel = true;
                mre.Set();
            };

            mre.WaitOne();

            Console.WriteLine("Exiting...");
            pc?.close();
        }

        private static RTCPeerConnection CreatePeerConnection()
        {
            var pc = new RTCPeerConnection();

            // Add a send-only audio track (this doesn't require any native libraries for encoding so is good for x-platform testing).
            AudioExtrasSource audioSource = new AudioExtrasSource(new AudioEncoder(), new AudioSourceOptions { AudioSource = AudioSourcesEnum.Music });
            audioSource.OnAudioSourceEncodedSample += pc.SendAudio;

            MediaStreamTrack audioTrack = new MediaStreamTrack(SDPWellKnownMediaFormatsEnum.PCMU);
            pc.addTrack(audioTrack);

            pc.OnAudioFormatsNegotiated += (formats) =>
                audioSource.SetAudioSourceFormat(formats.First());

            pc.onicecandidateerror += (candidate, error) => logger.LogWarning($"Error adding remote ICE candidate. {error} {candidate}");
            pc.OnTimeout += (mediaType) => logger.LogWarning($"Timeout for {mediaType}.");
            pc.oniceconnectionstatechange += (state) => logger.LogInformation($"ICE connection state changed to {state}.");
            pc.onsignalingstatechange += () => logger.LogInformation($"Signaling state changed to {pc.signalingState}.");

            pc.onconnectionstatechange += async (state) =>
            {
                logger.LogInformation($"Peer connection state changed to {state}.");

                if (state == RTCPeerConnectionState.disconnected || state == RTCPeerConnectionState.failed)
                {
                    pc.Close("remote disconnection");
                }

                if (state == RTCPeerConnectionState.connected)
                {
                    await audioSource.StartAudio();
                }
                else if (state == RTCPeerConnectionState.closed)
                {
                    await audioSource.CloseAudio();
                }
            };
            pc.OnReceiveReport += (re, media, rr) => logger.LogDebug($"RTCP Receive for {media} from {re}\n{rr.GetDebugSummary()}");
            pc.OnSendReport += (media, sr) => logger.LogDebug($"RTCP Send for {media}\n{sr.GetDebugSummary()}");
            pc.OnRtcpBye += (reason) => logger.LogDebug($"RTCP BYE receive, reason: {(string.IsNullOrWhiteSpace(reason) ? "<none>" : reason)}.");

            pc.onsignalingstatechange += () =>
            {
                if (pc.signalingState == RTCSignalingState.have_remote_offer)
                {
                    logger.LogTrace("Remote SDP:");
                    logger.LogTrace(pc.remoteDescription.sdp.ToString());
                }
                else if (pc.signalingState == RTCSignalingState.have_local_offer)
                {
                    logger.LogTrace("Local SDP:");
                    logger.LogTrace(pc.localDescription.sdp.ToString());
                }
            };

            return pc;
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
