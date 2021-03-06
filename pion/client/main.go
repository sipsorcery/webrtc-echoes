// +build !js

package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/pion/webrtc/v3"
)

var ECHO_TEST_SERVER_URL = "http://localhost:8080/offer"
var CONNECTION_ATTEMPT_TIMEOUT_SECONDS = 10
var SUCCESS_RETURN_VALUE = 0
var ERROR_RETURN_VALUE = 1

func main() {

	echoTestServerURL := ECHO_TEST_SERVER_URL
	if len(os.Args) > 1 {
		echoTestServerURL = os.Args[1]
		println("Echo test server URL set to:", echoTestServerURL)
	}

	connectResult := false
	defer func() {
		if err := recover(); err != nil {
			fmt.Println(err)
		}
		fmt.Println("Panic returning status:", ERROR_RETURN_VALUE)
		os.Exit(ERROR_RETURN_VALUE)
	}()

	ctx, cancel := context.WithTimeout(context.Background(),
		time.Duration(CONNECTION_ATTEMPT_TIMEOUT_SECONDS)*time.Second)
	defer cancel()

	// Everything below is the Pion WebRTC API! Thanks for using it ❤️.

	// Create a MediaEngine object to configure the supported codec
	m := webrtc.MediaEngine{}

	// Setup the codecs you want to use.
	// We'll use a VP8 and Opus but you can also define your own
	if err := m.RegisterCodec(webrtc.RTPCodecParameters{
		RTPCodecCapability: webrtc.RTPCodecCapability{MimeType: "video/VP8", ClockRate: 90000, Channels: 0, SDPFmtpLine: "", RTCPFeedback: nil},
		PayloadType:        96,
	}, webrtc.RTPCodecTypeVideo); err != nil {
		panic(err)
	}

	// Create the API object with the MediaEngine
	api := webrtc.NewAPI(webrtc.WithMediaEngine(&m))

	// Prepare the configuration
	config := webrtc.Configuration{
		ICEServers: []webrtc.ICEServer{
			{
				URLs: []string{"stun:stun.l.google.com:19302"},
			},
		},
	}
	// Create a new RTCPeerConnection
	peerConnection, err := api.NewPeerConnection(config)
	if err != nil {
		panic(err)
	}

	// Create Track that we send video back to browser on
	outputTrack, err := webrtc.NewTrackLocalStaticRTP(webrtc.RTPCodecCapability{MimeType: "video/vp8"}, "video", "pion")
	if err != nil {
		panic(err)
	}

	// Add this newly created track to the PeerConnection
	rtpSender, err := peerConnection.AddTrack(outputTrack)
	if err != nil {
		panic(err)
	}

	// Read incoming RTCP packets
	// Before these packets are retuned they are processed by interceptors. For things
	// like NACK this needs to be called.
	go func() {
		rtcpBuf := make([]byte, 1500)
		for {
			if _, _, rtcpErr := rtpSender.Read(rtcpBuf); rtcpErr != nil {
				return
			}
		}
	}()

	// Add ICE candidates to the local offer (simulates non-trickle).
	peerConnection.OnICECandidate(func(c *webrtc.ICECandidate) {
		if c == nil {
			//fmt.Println(peerConnection.LocalDescription())
		}
	})

	offer, err := peerConnection.CreateOffer(nil)
	if err != nil {
		panic(err)
	}

	// Sets the LocalDescription, and starts our UDP listeners
	if err = peerConnection.SetLocalDescription(offer); err != nil {
		panic(err)
	}

	// Set the handler for ICE connection state
	// This will notify you when the peer has connected/disconnected
	peerConnection.OnICEConnectionStateChange(func(connectionState webrtc.ICEConnectionState) {
		fmt.Printf("ICE connection state has changed %s.\n", connectionState.String())
	})

	peerConnection.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
		fmt.Printf("Peer connection state has changed to %s.\n", state.String())
		if state == webrtc.PeerConnectionStateConnected {
			connectResult = true
			peerConnection.Close()
			cancel()
		} else if state == webrtc.PeerConnectionStateDisconnected ||
			state == webrtc.PeerConnectionStateFailed ||
			state == webrtc.PeerConnectionStateClosed {
			connectResult = true
			cancel()
		}
	})

	peerConnection.OnSignalingStateChange(func(state webrtc.SignalingState) {
		fmt.Printf("Signaling state has changed to %s.\n", state.String())
	})

	// Create an answer
	/* answer, err := peerConnection.CreateAnswer(nil)
	if err != nil {
		panic(err)
	} */

	// Create channel that is blocked until ICE Gathering is complete
	gatherComplete := webrtc.GatheringCompletePromise(peerConnection)

	// Block until ICE Gathering is complete, disabling trickle ICE
	// we do this because we only can exchange one signaling message
	// in a production application you should exchange ICE Candidates via OnICECandidate
	<-gatherComplete

	fmt.Printf("Attempting to POST offer to %s.\n", echoTestServerURL)

	// POST the offer to the echo server.
	offerWithIce := peerConnection.LocalDescription()
	offerBuf := new(bytes.Buffer)
	json.NewEncoder(offerBuf).Encode(offerWithIce)
	req, err := http.NewRequest("POST", echoTestServerURL, offerBuf)
	req.Header.Set("Content-Type", "application/json")

	client := &http.Client{
		Timeout: time.Duration(CONNECTION_ATTEMPT_TIMEOUT_SECONDS) * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		panic(err)
	}
	defer resp.Body.Close()

	fmt.Println("POST offer response Status:", resp.Status)
	//fmt.Println("response Headers:", resp.Header)
	body, _ := ioutil.ReadAll(resp.Body)
	//fmt.Println("response Body:", string(body))

	// Set the remote SessionDescription
	var answer webrtc.SessionDescription
	err = json.NewDecoder(strings.NewReader(string(body))).Decode(&answer)
	if err != nil {
		panic(err)
	}

	err = peerConnection.SetRemoteDescription(answer)
	if err != nil {
		panic(err)
	}

	// Output the answer in base64 so we can paste it in browser
	//fmt.Println(signal.Encode(*peerConnection.LocalDescription()))

	select {
	case <-ctx.Done():
		fmt.Println("Context done.")
	}

	if connectResult {
		fmt.Println("Connection attempt successful returning status:", SUCCESS_RETURN_VALUE)
		os.Exit(SUCCESS_RETURN_VALUE)
	} else {
		fmt.Println("Connection attempt failed returning status:", ERROR_RETURN_VALUE)
		os.Exit(ERROR_RETURN_VALUE)
	}
}
