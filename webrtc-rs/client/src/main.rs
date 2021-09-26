use anyhow::Result;
use clap::{App, AppSettings, Arg};
use hyper::{Body, Client, Method, Request};
use interceptor::registry::Registry;
use std::io::Write;
use std::sync::Arc;
use tokio::time::Duration;
use webrtc::api::interceptor_registry::register_default_interceptors;
use webrtc::api::media_engine::{MediaEngine, MIME_TYPE_VP8};
use webrtc::api::APIBuilder;
use webrtc::media::rtp::rtp_codec::{RTCRtpCodecCapability, RTCRtpCodecParameters, RTPCodecType};
use webrtc::media::track::track_local::track_local_static_rtp::TrackLocalStaticRTP;
use webrtc::media::track::track_local::TrackLocal;
use webrtc::peer::configuration::RTCConfiguration;
use webrtc::peer::ice::ice_candidate::RTCIceCandidate;
use webrtc::peer::ice::ice_connection_state::RTCIceConnectionState;
use webrtc::peer::ice::ice_server::RTCIceServer;
use webrtc::peer::peer_connection_state::RTCPeerConnectionState;
use webrtc::peer::sdp::session_description::RTCSessionDescription;
use webrtc::peer::signaling_state::RTCSignalingState;

//const ECHO_TEST_SERVER_URL: &str = "http://localhost:8080/offer";
const CONNECTION_ATTEMPT_TIMEOUT_SECONDS: u64 = 10;
const SUCCESS_RETURN_VALUE: i32 = 0;
//const ERROR_RETURN_VALUE: i32 = 1;

#[tokio::main]
async fn main() -> Result<()> {
    let mut app = App::new("webrtc-rs echo client")
        .version("0.1.0")
        .author("Rain Liu <yliu@webrtc.rs>")
        .about("An example of webrtc-rs echo client.")
        .setting(AppSettings::DeriveDisplayOrder)
        .setting(AppSettings::SubcommandsNegateReqs)
        .arg(
            Arg::with_name("FULLHELP")
                .help("Prints more detailed help information")
                .long("fullhelp"),
        )
        .arg(
            Arg::with_name("debug")
                .long("debug")
                .short("d")
                .help("Prints debug log information"),
        )
        .arg(
            Arg::with_name("server_url")
                .default_value("http://localhost:8080/offer")
                .help("Echo HTTP server is hosted on."),
        );

    let matches = app.clone().get_matches();

    if matches.is_present("FULLHELP") {
        app.print_long_help().unwrap();
        std::process::exit(0);
    }

    let debug = matches.is_present("debug");
    if debug {
        env_logger::Builder::new()
            .format(|buf, record| {
                writeln!(
                    buf,
                    "{}:{} [{}] {} - {}",
                    record.file().unwrap_or("unknown"),
                    record.line().unwrap_or(0),
                    record.level(),
                    chrono::Local::now().format("%H:%M:%S.%6f"),
                    record.args()
                )
            })
            .filter(None, log::LevelFilter::Trace)
            .init();
    }

    let echo_test_server_url = matches.value_of("server_url").unwrap().to_owned();

    // Everything below is the WebRTC.rs API! Thanks for using it ❤️.

    // Create a MediaEngine object to configure the supported codec
    let mut m = MediaEngine::default();

    // Setup the codecs you want to use.
    // We'll use a VP8 but you can also define your own
    m.register_codec(
        RTCRtpCodecParameters {
            capability: RTCRtpCodecCapability {
                mime_type: MIME_TYPE_VP8.to_owned(),
                clock_rate: 90000,
                channels: 0,
                sdp_fmtp_line: "".to_owned(),
                rtcp_feedback: vec![],
            },
            payload_type: 96,
            ..Default::default()
        },
        RTPCodecType::Video,
    )?;

    // Create a InterceptorRegistry. This is the user configurable RTP/RTCP Pipeline.
    // This provides NACKs, RTCP Reports and other features. If you use `webrtc.NewPeerConnection`
    // this is enabled by default. If you are manually managing You MUST create a InterceptorRegistry
    // for each PeerConnection.
    let mut registry = Registry::new();

    // Use the default set of Interceptors
    registry = register_default_interceptors(registry, &mut m)?;

    // Create the API object with the MediaEngine
    let api = APIBuilder::new()
        .with_media_engine(m)
        .with_interceptor_registry(registry)
        .build();

    // Prepare the configuration
    let config = RTCConfiguration {
        ice_servers: vec![RTCIceServer {
            urls: vec!["stun:stun.l.google.com:19302".to_owned()],
            ..Default::default()
        }],
        ..Default::default()
    };

    // Create a new RTCPeerConnection
    let peer_connection = Arc::new(api.new_peer_connection(config).await?);

    // Create Track that we send video back to browser on
    let output_track = Arc::new(TrackLocalStaticRTP::new(
        RTCRtpCodecCapability {
            mime_type: MIME_TYPE_VP8.to_owned(),
            ..Default::default()
        },
        "video".to_owned(),
        "webrtc-rs".to_owned(),
    ));

    // Add this newly created track to the PeerConnection
    let rtp_sender = peer_connection
        .add_track(Arc::clone(&output_track) as Arc<dyn TrackLocal + Send + Sync>)
        .await?;

    // Read incoming RTCP packets
    // Before these packets are returned they are processed by interceptors. For things
    // like NACK this needs to be called.
    tokio::spawn(async move {
        let mut rtcp_buf = vec![0u8; 1500];
        while let Ok((_, _)) = rtp_sender.read(&mut rtcp_buf).await {}
        Result::<()>::Ok(())
    });

    // Add ICE candidates to the local offer (simulates non-trickle).
    peer_connection
        .on_ice_candidate(Box::new(move |c: Option<RTCIceCandidate>| {
            if c.is_none() {
                //println!(peer_connection.LocalDescription())
            }
            Box::pin(async {})
        }))
        .await;

    // Create an offer to send to the other process
    let offer = peer_connection.create_offer(None).await?;

    // Sets the LocalDescription, and starts our UDP listeners
    peer_connection.set_local_description(offer).await?;

    // Set the handler for ICE connection state
    // This will notify you when the peer has connected/disconnected
    peer_connection
        .on_ice_connection_state_change(Box::new(|connection_state: RTCIceConnectionState| {
            println!("ICE connection state has changed {}.", connection_state);
            Box::pin(async {})
        }))
        .await;

    let (done_tx, mut done_rx) = tokio::sync::mpsc::channel::<()>(1);
    let done_tx = Arc::new(done_tx);
    peer_connection
        .on_peer_connection_state_change(Box::new(move |state: RTCPeerConnectionState| {
            println!("Peer connection state has changed to {}.", state);

            let done_tx2 = Arc::clone(&done_tx);
            Box::pin(async move {
                if state == RTCPeerConnectionState::Connected {
                    let _ = done_tx2.send(()).await;
                } else if state == RTCPeerConnectionState::Disconnected
                    || state == RTCPeerConnectionState::Failed
                    || state == RTCPeerConnectionState::Closed
                {
                    std::process::exit(SUCCESS_RETURN_VALUE);
                }
            })
        }))
        .await;

    peer_connection
        .on_signaling_state_change(Box::new(|state: RTCSignalingState| {
            println!("Signaling state has changed to {}.", state);
            Box::pin(async {})
        }))
        .await;

    // Create channel that is blocked until ICE Gathering is complete
    let mut gather_complete = peer_connection.gathering_complete_promise().await;

    // Block until ICE Gathering is complete, disabling trickle ICE
    // we do this because we only can exchange one signaling message
    // in a production application you should exchange ICE Candidates via OnICECandidate
    let _ = gather_complete.recv().await;

    println!("Attempting to POST offer to {}.", echo_test_server_url);

    // POST the offer to the echo server.
    let offer_with_ice = peer_connection.local_description().await.unwrap();
    let offer_buf = serde_json::to_string(&offer_with_ice)?;

    let req = Request::builder()
        .method(Method::POST)
        .uri(echo_test_server_url.to_owned())
        .header("content-type", "application/json")
        .body(Body::from(offer_buf))?;

    let resp = Client::new().request(req).await?;

    println!("POST offer response Status: {}", resp.status());
    let buf = hyper::body::to_bytes(resp.into_body()).await?;
    let sdp_str = std::str::from_utf8(&buf)?;
    let answer = serde_json::from_str::<RTCSessionDescription>(sdp_str)?;

    // Set the remote SessionDescription
    peer_connection.set_remote_description(answer).await?;

    let timeout = tokio::time::sleep(Duration::from_secs(CONNECTION_ATTEMPT_TIMEOUT_SECONDS));
    tokio::pin!(timeout);
    tokio::select! {
        _ = timeout.as_mut() =>{
            println!("Peer Connnection TimeOut! Exit echo client now...");
        }
        _ = done_rx.recv() => {
            println!("Peer Connnection Connected! Exit echo client now...");
        }
    }

    peer_connection.close().await?;

    Ok(())
}
