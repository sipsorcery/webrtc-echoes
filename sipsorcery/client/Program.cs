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
// 14 Apr 2021  Aaron Clauson   Added data channel support.
//
// License: 
// BSD 3-Clause "New" or "Revised" License, see included LICENSE.md file.
//-----------------------------------------------------------------------------

using System;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using CommandLine;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using Serilog;
using Serilog.Events;
using Serilog.Extensions.Logging;
using SIPSorcery.Net;
using SIPSorcery.Sys;
using SIPSorceryMedia.Abstractions;

namespace webrtc_echo
{
    public enum WebRTCTestTypes
    {
        PeerConnection = 0,
        DataChannelEcho = 1
    }

    public class Options
    {
        public const string DEFAULT_ECHO_SERVER_URL = "http://localhost:8080/offer";
        public const LogEventLevel DEFAULT_VERBOSITY = LogEventLevel.Debug;
        public const int TEST_TIMEOUT_SECONDS = 10;

        [Option('s', "server", Required = false, Default = DEFAULT_ECHO_SERVER_URL,
            HelpText = "Perform a data channel echo test where the success condition is receiving back a data channel message.")]
        public string ServerUrl { get; set; }

        [Option('t', "test", Required = false, Default = WebRTCTestTypes.PeerConnection,
            HelpText = "The type of test to perform. Options are 'PeerConnection' or 'DataChannelEcho'.")]
        public WebRTCTestTypes TestType { get; set; }

        [Option("timeout", Required = false, Default = TEST_TIMEOUT_SECONDS,
            HelpText = "Timeout in seconds to close the peer connection. Set to 0 for no timeout.")]
        public int TestTimeoutSeconds { get; set; }

        [Option('v', "verbosity", Required = false, Default = DEFAULT_VERBOSITY,
            HelpText = "The log level verbosity (0=Verbose, 1=Debug, 2=Info, 3=Warn...).")]
        public LogEventLevel Verbosity { get; set; }
    }

    class Program
    {
        private const int SUCCESS_RESULT = 0;
        private const int FAILURE_RESULT = 1;

        private static Microsoft.Extensions.Logging.ILogger logger = NullLogger.Instance;

        static async Task<int> Main(string[] args)
        {
            Console.WriteLine("Starting webrtc echo test client.");

            WebRTCTestTypes testType = WebRTCTestTypes.PeerConnection;
            string echoServerUrl = Options.DEFAULT_ECHO_SERVER_URL;
            LogEventLevel verbosity = Options.DEFAULT_VERBOSITY;
            int pcTimeout = Options.TEST_TIMEOUT_SECONDS;

            if (args?.Length == 1)
            {
                echoServerUrl = args[0];
            }
            else if (args?.Length > 1)
            {
                Options opts = null;
                var parseResult = Parser.Default.ParseArguments<Options>(args)
                    .WithParsed(o => opts = o);

                testType = opts != null ? opts.TestType : testType;
                echoServerUrl = opts != null && !string.IsNullOrEmpty(opts.ServerUrl) ? opts.ServerUrl : echoServerUrl;
                verbosity = opts != null ? opts.Verbosity : verbosity;
                pcTimeout = opts != null ? opts.TestTimeoutSeconds : pcTimeout;
            }

            logger = AddConsoleLogger(verbosity);

            logger.LogDebug($"Performing test {testType} to {echoServerUrl}.");

            var pc = await CreatePeerConnection();
            var offer = pc.createOffer();
            await pc.setLocalDescription(offer);

            bool success = false;
            ManualResetEventSlim testComplete = new ManualResetEventSlim();

            var dc = pc.DataChannels.FirstOrDefault();

            if (dc != null)
            {
                var pseudo = Crypto.GetRandomString(5);

                dc.onopen += () =>
                {
                    logger.LogInformation($"Data channel {dc.label}, stream ID {dc.id} opened.");
                    dc.send(pseudo);
                };

                dc.onmessage += (dc, proto, data) =>
                {
                    string echoMsg = Encoding.UTF8.GetString(data);
                    logger.LogDebug($"data channel onmessage {proto}: {echoMsg}.");

                    if (echoMsg == pseudo)
                    {
                        logger.LogInformation($"Data channel echo test success.");

                        if (testType == WebRTCTestTypes.DataChannelEcho)
                        {
                            success = true;
                            testComplete.Set();
                        }
                    }
                    else
                    {
                        logger.LogWarning($"Data channel echo test failed, echoed message of {echoMsg} did not match original of {pseudo}.");
                    }
                };
            }

            pc.onconnectionstatechange += (state) =>
            {
                logger.LogInformation($"Peer connection state changed to {state}.");

                if (state == RTCPeerConnectionState.disconnected || state == RTCPeerConnectionState.failed)
                {
                    pc.Close("remote disconnection");
                }
                else if (state == RTCPeerConnectionState.connected &&
                    testType == WebRTCTestTypes.PeerConnection)
                {
                    success = true;
                    testComplete.Set();
                }
            };

            logger.LogInformation($"Posting offer to {echoServerUrl}.");

            var httpClient = new HttpClient();
            var content = new StringContent(offer.toJSON(), Encoding.UTF8, "application/json");
            var response = await httpClient.PostAsync(echoServerUrl, content);
            var answerStr = await response.Content.ReadAsStringAsync();

            if (RTCSessionDescriptionInit.TryParse(answerStr, out var answerInit))
            {
                logger.LogDebug($"SDP answer: {answerInit.sdp}");

                var setAnswerResult = pc.setRemoteDescription(answerInit);
                if (setAnswerResult != SetDescriptionResultEnum.OK)
                {
                    logger.LogWarning($"Set remote description failed {setAnswerResult}.");
                }
            }
            else
            {
                logger.LogWarning($"Failed to parse SDP answer from echo server: {answerStr}.");
            }

            pc.OnClosed += testComplete.Set;

            logger.LogDebug($"Test timeout is {pcTimeout} seconds.");
            testComplete.Wait(pcTimeout * 1000);

            if (!pc.IsClosed)
            {
                pc.close();
                await Task.Delay(1000);
            }

            logger.LogInformation($"{testType} test success {success }.");

            return (success) ? SUCCESS_RESULT : FAILURE_RESULT;
        }

        private static async Task<RTCPeerConnection> CreatePeerConnection()
        {
            RTCConfiguration config = new RTCConfiguration
            {
                X_DisableExtendedMasterSecretKey = true
            };

            var pc = new RTCPeerConnection(config);

            MediaStreamTrack audioTrack = new MediaStreamTrack(SDPWellKnownMediaFormatsEnum.PCMU);
            pc.addTrack(audioTrack);

            var dc = await pc.createDataChannel("sipsocery-dc");

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
