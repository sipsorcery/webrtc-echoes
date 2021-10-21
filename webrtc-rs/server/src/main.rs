use anyhow::Result;
use async_trait::async_trait;
use clap::{App, AppSettings, Arg};
use hyper::service::{make_service_fn, service_fn};
use hyper::{Body, Method, Request, Response, Server, StatusCode};
use interceptor::registry::Registry;
use std::any::Any;
use std::io::Write;
use std::net::SocketAddr;
use std::str::FromStr;
use std::sync::Arc;
use tokio::sync::Mutex;
use tokio_util::codec::{BytesCodec, FramedRead};
use webrtc::api::interceptor_registry::register_default_interceptors;
use webrtc::api::media_engine::MediaEngine;
use webrtc::api::APIBuilder;
use webrtc::media::rtp::rtp_codec::{RTCRtpCodecParameters, RTPCodecType};
use webrtc::media::rtp::rtp_receiver::RTCRtpReceiver;
use webrtc::media::track::track_local::{TrackLocal, TrackLocalContext};
use webrtc::media::track::track_remote::TrackRemote;
use webrtc::peer::configuration::RTCConfiguration;
use webrtc::peer::ice::ice_connection_state::RTCIceConnectionState;
use webrtc::peer::ice::ice_server::RTCIceServer;
use webrtc::peer::peer_connection::RTCPeerConnection;
use webrtc::peer::peer_connection_state::RTCPeerConnectionState;
use webrtc::peer::sdp::session_description::RTCSessionDescription;
use webrtc::peer::signaling_state::RTCSignalingState;

#[macro_use]
extern crate lazy_static;

lazy_static! {
    static ref PEER_CONNECTIONS_MUTEX: Arc<Mutex<Vec<Arc<RTCPeerConnection>>>> =
        Arc::new(Mutex::new(vec![]));
    static ref HTML_FILE: Arc<Mutex<Option<String>>> = Arc::new(Mutex::new(None));
}

static NOTFOUND: &[u8] = b"Not Found";

/// HTTP status code 404
fn not_found() -> Response<Body> {
    Response::builder()
        .status(StatusCode::NOT_FOUND)
        .body(NOTFOUND.into())
        .unwrap()
}

async fn simple_file_send(filename: String) -> Result<Response<Body>, hyper::Error> {
    // Serve a file by asynchronously reading it by chunks using tokio-util crate.

    if let Ok(file) = tokio::fs::File::open(filename).await {
        let stream = FramedRead::new(file, BytesCodec::new());
        let body = Body::wrap_stream(stream);
        return Ok(Response::new(body));
    }

    Ok(not_found())
}

// HTTP Listener to get ICE Credentials/Candidate from remote Peer
async fn remote_handler(req: Request<Body>) -> Result<Response<Body>, hyper::Error> {
    /*let pc = {
        let pcm = PEER_CONNECTION_MUTEX.lock().await;
        pcm.clone().unwrap()
    };*/
    let html_file = {
        let html_file = HTML_FILE.lock().await;
        html_file.clone().unwrap()
    };

    match (req.method(), req.uri().path()) {
        (&Method::GET, "/") | (&Method::GET, "/index.html") => simple_file_send(html_file).await,

        (&Method::POST, "/offer") => process_offer(req).await,

        // Return the 404 Not Found for other routes.
        _ => {
            let mut not_found = Response::default();
            *not_found.status_mut() = StatusCode::NOT_FOUND;
            Ok(not_found)
        }
    }
}

async fn process_offer(req: Request<Body>) -> Result<Response<Body>, hyper::Error> {
    let sdp_str = match std::str::from_utf8(&hyper::body::to_bytes(req.into_body()).await?) {
        Ok(s) => s.to_owned(),
        Err(err) => panic!("{}", err),
    };
    let offer = match serde_json::from_str::<RTCSessionDescription>(&sdp_str) {
        Ok(s) => s,
        Err(err) => panic!("{}", err),
    };

    // Everything below is the WebRTC-rs API! Thanks for using it ❤️.

    // Create a MediaEngine object to configure the supported codec
    let mut m = MediaEngine::default();

    match m.register_default_codecs() {
        Ok(()) => {}
        Err(err) => panic!("{}", err),
    };

    // Create a InterceptorRegistry. This is the user configurable RTP/RTCP Pipeline.
    // This provides NACKs, RTCP Reports and other features. If you use `webrtc.NewPeerConnection`
    // this is enabled by default. If you are manually managing You MUST create a InterceptorRegistry
    // for each PeerConnection.
    let mut registry = Registry::new();

    // Use the default set of Interceptors
    registry = match register_default_interceptors(registry, &mut m) {
        Ok(r) => r,
        Err(err) => panic!("{}", err),
    };

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
    let pc = match api.new_peer_connection(config).await {
        Ok(pc) => pc,
        Err(err) => panic!("{}", err),
    };
    let pc = Arc::new(pc);

    {
        let mut pcs = PEER_CONNECTIONS_MUTEX.lock().await;
        pcs.push(Arc::clone(&pc));
    }

    // // Set the handler for ICE connection state
    // // This will notify you when the peer has connected/disconnected
    let stats_id = pc.get_stats_id().to_owned();
    pc.on_ice_connection_state_change(Box::new(move |connection_state: RTCIceConnectionState| {
        println!(
            "ICE connection {} state has changed to {}.",
            stats_id, connection_state
        );

        if connection_state == RTCIceConnectionState::Failed {
            let stats_id2 = stats_id.clone();
            tokio::spawn(async move {
                let mut pcs = PEER_CONNECTIONS_MUTEX.lock().await;
                for pc in &*pcs {
                    if pc.get_stats_id() == stats_id2 {
                        println!("Peer Connection {} is closing", stats_id2);
                        if let Err(err) = pc.close().await {
                            println!(
                                "Peer Connection {} closing got err: {}",
                                pc.get_stats_id(),
                                err
                            );
                        } else {
                            println!("Peer Connection {} is closed", stats_id2);
                        }
                    }
                }
                pcs.retain(|x| x.get_stats_id() != stats_id2);
                println!("Peer Connection {} is removed from PCS", stats_id2);
            });
        }
        Box::pin(async {})
    }))
    .await;

    let audio_track = Arc::new(EchoTrack::new(
        "audio".to_owned(),
        "webrtc-rs".to_owned(),
        RTPCodecType::Audio,
    ));
    match pc
        .add_track(Arc::clone(&audio_track) as Arc<dyn TrackLocal + Send + Sync>)
        .await
    {
        Ok(_) => {}
        Err(err) => panic!("{}", err),
    };

    let video_track = Arc::new(EchoTrack::new(
        "video".to_owned(),
        "webrtc-rs".to_owned(),
        RTPCodecType::Video,
    ));
    match pc
        .add_track(Arc::clone(&video_track) as Arc<dyn TrackLocal + Send + Sync>)
        .await
    {
        Ok(_) => {}
        Err(err) => panic!("{}", err),
    };

    pc.on_track(Box::new(
        move |track: Option<Arc<TrackRemote>>, _receiver: Option<Arc<RTCRtpReceiver>>| {
            let audio_track2 = Arc::clone(&audio_track);
            let video_track2 = Arc::clone(&video_track);
            Box::pin(async move {
                if let Some(track) = track {
                    let mime_type = track.codec().await.capability.mime_type;
                    let out_track = if mime_type.starts_with("audio") {
                        audio_track2
                    } else {
                        video_track2
                    };

                    println!(
                        "Track has started, of type {}: {}",
                        track.payload_type(),
                        mime_type
                    );
                    tokio::spawn(async move {
                        let (ssrc, write_stream) = {
                            let ctx = out_track.ctx.lock().await;
                            (ctx[0].ssrc(), ctx[0].write_stream())
                        };
                        while let Ok((mut rtp, _)) = track.read_rtp().await {
                            rtp.header.ssrc = ssrc;
                            if let Some(ws) = &write_stream {
                                if let Err(err) = ws.write_rtp(&rtp).await {
                                    println!("write_stream.write_rtp err: {}", err);
                                    break;
                                }
                            } else {
                                println!("write_stream is none");
                                break;
                            }
                        }

                        println!(
                            "Track has ended, of type {}: {}",
                            track.payload_type(),
                            mime_type
                        );
                    });
                }
            })
        },
    ))
    .await;

    let stats_id = pc.get_stats_id().to_owned();
    pc.on_peer_connection_state_change(Box::new(move |state: RTCPeerConnectionState| {
        println!(
            "Peer connection {} state has changed to {}.",
            stats_id, state
        );
        Box::pin(async {})
    }))
    .await;

    let stats_id = pc.get_stats_id().to_owned();
    pc.on_signaling_state_change(Box::new(move |state: RTCSignalingState| {
        println!("Signaling state {} has changed to {}.", stats_id, state);
        Box::pin(async {})
    }))
    .await;

    if let Err(err) = pc.set_remote_description(offer).await {
        panic!("{}", err);
    }

    let answer = match pc.create_answer(None).await {
        Ok(answer) => answer,
        Err(err) => panic!("{}", err),
    };

    if let Err(err) = pc.set_local_description(answer).await {
        panic!("{}", err);
    }

    let payload = if let Some(local_desc) = pc.local_description().await {
        match serde_json::to_string(&local_desc) {
            Ok(p) => p,
            Err(err) => panic!("{}", err),
        }
    } else {
        panic!("generate local_description failed!");
    };

    let mut response = match Response::builder()
        .header("content-type", "application/json")
        .body(Body::from(payload))
    {
        Ok(res) => res,
        Err(err) => panic!("{}", err),
    };

    *response.status_mut() = StatusCode::OK;
    Ok(response)
}

#[tokio::main]
async fn main() -> Result<()> {
    let mut app = App::new("webrtc-rs echo server")
        .version("0.1.0")
        .author("Rain Liu <yliu@webrtc.rs>")
        .about("An example of webrtc-rs echo server.")
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
            Arg::with_name("host")
                .takes_value(true)
                .default_value("0.0.0.0:8080")
                .long("host")
                .help("Echo HTTP server is hosted on."),
        )
        /*.arg(
            Arg::with_name("cert-file")
                .takes_value(true)
                .long("cert-file")
                .help("SSL certificate file (for HTTPS)."),
        )
        .arg(
            Arg::with_name("key-file")
                .takes_value(true)
                .long("key-file")
                .help("SSL key file (for HTTPS)."),
        )*/
        .arg(
            Arg::with_name("html-file")
                .takes_value(true)
                .default_value("../../html/index.html")
                .long("html-file")
                .help("Html file to be displayed."),
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

    let host = matches.value_of("host").unwrap().to_owned();
    //let certFile = matches.value_of("cert-file");
    //let keyFile = matches.value_of("key-file");
    let html_file = matches.value_of("html-file").unwrap().to_owned();
    {
        let mut hf = HTML_FILE.lock().await;
        *hf = Some(html_file);
    }

    println!("Listening on {}...", host);

    let addr = SocketAddr::from_str(&host).unwrap();
    let service = make_service_fn(|_| async { Ok::<_, hyper::Error>(service_fn(remote_handler)) });
    let server = Server::bind(&addr).serve(service);

    // Run this server for... forever!
    if let Err(e) = server.await {
        eprintln!("server error: {}", e);
    }

    Ok(())
}

// EchoTrack just passes through RTP packets
// in a real application you will want to confirm that
// the receiver supports the codec and updating PayloadTypes if needed
struct EchoTrack {
    id: String,
    stream_id: String,
    kind: RTPCodecType,
    ctx: Arc<Mutex<Vec<TrackLocalContext>>>,
}

impl EchoTrack {
    fn new(id: String, stream_id: String, kind: RTPCodecType) -> Self {
        EchoTrack {
            id,
            stream_id,
            kind,
            ctx: Arc::new(Mutex::new(vec![])),
        }
    }
}

#[async_trait]
impl TrackLocal for EchoTrack {
    async fn bind(&self, c: &TrackLocalContext) -> std::result::Result<RTCRtpCodecParameters, webrtc::Error> {
        let mut ctx = self.ctx.lock().await;
        ctx.push(c.clone());

        Ok(c.codec_parameters()[0].clone())
    }

    async fn unbind(&self, _: &TrackLocalContext) -> std::result::Result<(), webrtc::Error> {
        Ok(())
    }

    fn id(&self) -> &str {
        self.id.as_str()
    }

    fn stream_id(&self) -> &str {
        self.stream_id.as_str()
    }

    fn kind(&self) -> RTPCodecType {
        self.kind
    }

    fn as_any(&self) -> &dyn Any {
        self
    }
}
