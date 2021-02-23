import argparse
import asyncio
import logging

import aiohttp

from aiortc import RTCPeerConnection, RTCSessionDescription
from aiortc.contrib.media import MediaBlackhole, MediaPlayer, MediaRecorder


async def send_offer(
    url: str,
    offer: RTCSessionDescription,
) -> RTCSessionDescription:
    offer_data = {"sdp": offer.sdp, "type": offer.type}
    async with aiohttp.ClientSession() as client:
        async with client.post(url, json=offer_data) as response:
            answer_data = await response.json()
            return RTCSessionDescription(
                sdp=answer_data["sdp"], type=answer_data["type"]
            )


async def run(*, pc, player, recorder, url):
    connected = asyncio.Event()

    @pc.on("connectionstatechange")
    def on_connectionstatechange():
        print("Connection state %s" % pc.connectionState)
        if pc.connectionState == "connected":
            connected.set()

    @pc.on("track")
    def on_track(track):
        print("Receiving %s" % track.kind)
        recorder.addTrack(track)

    # add local media
    if player and player.audio:
        pc.addTrack(player.audio)
    else:
        pc.addTransceiver("audio")

    if player and player.video:
        pc.addTrack(player.video)

    # send offer
    await pc.setLocalDescription(await pc.createOffer())
    answer = await send_offer(url, pc.localDescription)

    # apply answer
    await pc.setRemoteDescription(answer)

    # wait for connected state
    await connected.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="WebRTC Echo Client demo")
    parser.add_argument("--play-from", help="Read the media from a file and sent it."),
    parser.add_argument("--record-to", help="Write received media to a file."),
    parser.add_argument("--verbose", "-v", action="count")
    parser.add_argument("url", help="The URL to which the SDP offer is sent."),
    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)

    # create peer connection
    pc = RTCPeerConnection()

    # create media source
    if args.play_from:
        player = MediaPlayer(args.play_from)
    else:
        player = None

    # create media sink
    if args.record_to:
        recorder = MediaRecorder(args.record_to)
    else:
        recorder = MediaBlackhole()

    # run event loop
    loop = asyncio.get_event_loop()
    try:
        loop.run_until_complete(
            asyncio.wait_for(
                run(
                    pc=pc,
                    player=player,
                    recorder=recorder,
                    url=args.url,
                ),
                timeout=10,
            )
        )
        if args.play_from or args.record_to:
            loop.run_forever()
    except KeyboardInterrupt:
        pass
    finally:
        # cleanup
        loop.run_until_complete(recorder.stop())
        loop.run_until_complete(pc.close())
