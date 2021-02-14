package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"strings"
	"sync"

	"github.com/pion/webrtc/v3"
)

var (
	pcs     []*webrtc.PeerConnection
	pcsLock sync.RWMutex
)

func offer(w http.ResponseWriter, r *http.Request) {
	var offer webrtc.SessionDescription
	if err := json.NewDecoder(r.Body).Decode(&offer); err != nil {
		log.Fatal(err)
	}

	pc, err := webrtc.NewPeerConnection(webrtc.Configuration{})
	if err != nil {
		log.Fatal(err)
	}

	pcsLock.Lock()
	pcs = append(pcs, pc)
	pcsLock.Unlock()

	// // Set the handler for ICE connection state
	// // This will notify you when the peer has connected/disconnected
	pc.OnICEConnectionStateChange(func(connectionState webrtc.ICEConnectionState) {
		fmt.Printf("ICE connection state has changed to %s.\n", connectionState.String())

		if connectionState == webrtc.ICEConnectionStateFailed {
			pcsLock.Lock()
			defer pcsLock.Unlock()

			for i := range pcs {
				if pcs[i] == pc {
					if err = pc.Close(); err != nil {
						log.Fatal(err)
					}

					pcs = append(pcs[:i], pcs[i+1:]...)
				}
			}
		}
	})

	audioTrack := &echoTrack{id: "audio", streamID: "pion", kind: webrtc.RTPCodecTypeAudio}
	if _, err = pc.AddTrack(audioTrack); err != nil {
		log.Fatal(err)
	}

	videoTrack := &echoTrack{id: "video", streamID: "pion", kind: webrtc.RTPCodecTypeVideo}
	if _, err = pc.AddTrack(videoTrack); err != nil {
		log.Fatal(err)
	}

	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
		outTrack := videoTrack
		if strings.HasPrefix(track.Codec().MimeType, "audio") {
			outTrack = audioTrack
		}

		fmt.Printf("Track has started, of type %d: %s \n", track.PayloadType(), track.Codec().MimeType)
		for {
			rtp, _, err := track.ReadRTP()
			if err != nil {
				if err == io.EOF {
					return
				}

				log.Fatal(err)
			}

			rtp.SSRC = uint32(outTrack.ctx.SSRC())
			if _, err = outTrack.ctx.WriteStream().WriteRTP(&rtp.Header, rtp.Payload); err != nil {
				log.Fatal(err)
			}
		}

	})

	pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
		fmt.Printf("Peer connection state has changed to %s.\n", state.String())
	})

	pc.OnSignalingStateChange(func(state webrtc.SignalingState) {
		fmt.Printf("Signaling state has changed to %s.\n", state.String())
	})

	if err := pc.SetRemoteDescription(offer); err != nil {
		log.Fatal(err)
	}

	answer, err := pc.CreateAnswer(nil)
	if err != nil {
		log.Fatal(err)
	} else if err = pc.SetLocalDescription(answer); err != nil {
		log.Fatal(err)
	}

	response, err := json.Marshal(pc.LocalDescription())
	if err != nil {
		log.Fatal(err)
	}

	w.Header().Set("Content-Type", "application/json")
	if _, err := w.Write(response); err != nil {
		log.Fatal(err)
	}

}

func main() {
	certFile := flag.String("cert-file", "", "SSL certificate file (for HTTPS)")
	keyFile := flag.String("key-file", "", "SSL key file (for HTTPS)")
	host := flag.String("host", "0.0.0.0", "Host for HTTP server (default: 0.0.0.0)")
	port := flag.Int("port", 8080, "Port for HTTP server (default: 8080)")
	flag.Parse()

	pcs = []*webrtc.PeerConnection{}

	http.Handle("/", http.FileServer(http.Dir("../html")))
	http.HandleFunc("/offer", offer)

	addr := fmt.Sprintf("%s:%d", *host, *port)

	fmt.Printf("Listening on %s...\n", addr)

	if *keyFile != "" && *certFile != "" {
		log.Fatal(http.ListenAndServeTLS(addr, *certFile, *keyFile, nil))
	} else {
		log.Fatal(http.ListenAndServe(addr, nil))
	}
}

// echoTrack just passes through RTP packets
// in a real application you will want to confirm that
// the receiver supports the codec and updating PayloadTypes if needed
type echoTrack struct {
	id, streamID string
	kind         webrtc.RTPCodecType
	ctx          webrtc.TrackLocalContext
}

func (e *echoTrack) Bind(c webrtc.TrackLocalContext) (webrtc.RTPCodecParameters, error) {
	e.ctx = c
	return c.CodecParameters()[0], nil
}

func (e *echoTrack) Unbind(webrtc.TrackLocalContext) error { return nil }
func (e *echoTrack) ID() string                            { return e.id }
func (e *echoTrack) StreamID() string                      { return e.streamID }
func (e *echoTrack) Kind() webrtc.RTPCodecType             { return e.kind }
