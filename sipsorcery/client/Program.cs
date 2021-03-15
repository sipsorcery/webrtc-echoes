//-----------------------------------------------------------------------------
// Filename: Program.cs
//
// Description: Implements a WebRTC Echo Test Client suitable for interoperability
// testing as per specification at:
// https://github.com/sipsorcery/webrtc-echoes/blob/master/doc/EchoTestSpecification.md
//
// Author(s):
// Aaron Clauson (aaron@sipsorcery.com)
//
// History:
// 19 Feb 2021	Aaron Clauson	Created, Dublin, Ireland.
//
// License: 
// BSD 3-Clause "New" or "Revised" License, see included LICENSE.md file.
//-----------------------------------------------------------------------------

using System;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
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
        private const int SUCCESS_RESULT = 0;
        private const int FAILURE_RESULT = 1;

        private const string DEFAULT_ECHO_SERVER_URL = "http://localhost:8080/offer";

        private static Microsoft.Extensions.Logging.ILogger logger = NullLogger.Instance;

        static async Task<int> Main(string[] args)
        {
            Console.WriteLine("Starting webrtc echo test client.");

            string echoServerUrl = DEFAULT_ECHO_SERVER_URL;
            if (args?.Length > 0)
            {
                echoServerUrl = args[0];
            }

            logger = AddConsoleLogger(LogEventLevel.Verbose);

            var pc = CreatePeerConnection();
            var offer = pc.createOffer(null);
            await pc.setLocalDescription(offer);

            bool didConnect = false;
            TaskCompletionSource<int> connectResult = new TaskCompletionSource<int>();

            pc.onconnectionstatechange += (state) =>
            {
                logger.LogInformation($"Peer connection state changed to {state}.");

                if (state == RTCPeerConnectionState.disconnected || state == RTCPeerConnectionState.failed)
                {
                    pc.Close("remote disconnection");
                }
                else if (state == RTCPeerConnectionState.connected)
                {
                    didConnect = true;
                    pc.Close("normal");
                }
                else if (state == RTCPeerConnectionState.closed)
                {
                    connectResult.SetResult(didConnect ? SUCCESS_RESULT : FAILURE_RESULT);
                }
            };

            logger.LogInformation($"Posting offer to {echoServerUrl}.");

            var httpClient = new HttpClient();
            var content = new StringContent(offer.toJSON(), Encoding.UTF8, "application/json");
            var response = await httpClient.PostAsync(echoServerUrl, content);
            var answerStr = await response.Content.ReadAsStringAsync();

            logger.LogDebug($"SDP answer: {answerStr}");

            if (RTCSessionDescriptionInit.TryParse(answerStr, out var answerInit))
            {
                var setAnswerResult = pc.setRemoteDescription(answerInit);
                if (setAnswerResult != SetDescriptionResultEnum.OK)
                {
                    logger.LogWarning($"Set remote description failed {setAnswerResult}.");
                }
            }
            else
            {
                logger.LogWarning("Failed to parse SDP answer from echo server.");
            }

            var result = await connectResult.Task;

            logger.LogInformation($"Connection result {result}.");

            return result;
        }

        private static RTCPeerConnection CreatePeerConnection()
        {
            var pc = new RTCPeerConnection();

            MediaStreamTrack audioTrack = new MediaStreamTrack(SDPWellKnownMediaFormatsEnum.PCMU);
            pc.addTrack(audioTrack);

            pc.onicecandidateerror += (candidate, error) => logger.LogWarning($"Error adding remote ICE candidate. {error} {candidate}");
            pc.OnTimeout += (mediaType) => logger.LogWarning($"Timeout for {mediaType}.");
            pc.oniceconnectionstatechange += (state) => logger.LogInformation($"ICE connection state changed to {state}.");
            pc.onsignalingstatechange += () => logger.LogInformation($"Signaling state changed to {pc.signalingState}.");
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
