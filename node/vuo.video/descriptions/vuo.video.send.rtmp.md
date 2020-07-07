Streams video over the internet using the Real-Time Messaging Protocol (RTMP).

You can use RTMP to broadcast video (and optionally audio) using relay servers such as [Twitch](https://www.twitch.tv/), [Wowza](https://www.wowza.com/), [Restream](https://restream.io/), [YouTube](https://studio.youtube.com/channel), [Facebook](https://www.facebook.com/live/producer), and [Periscope](https://www.periscope.tv/account/producer), or by [setting up your own Nginx relay server](https://github.com/arut/nginx-rtmp-module).

   - `Send Image`, `Send Audio` — Streams an image or audio to an RTMP server.
   - `URL` — The address of the server to send to.  For example, `rtmp://127.0.0.1/live/`.
   - `Stream Key` — The server's private, unique identifier for this streaming channel.  (Some RTMP servers require this, some don't.)
   - `Username`, `Password` — The credentials to authenticate with the server.  (Some RTMP servers require this, some don't.)
   - `Quality` — Ranges from 0 (lowest quality and bandwidth) to 1 (highest quality and bandwidth).

RTMP expects video frames at a constant framerate, so you should use [Fire on Display Refresh](vuo-node://vuo.event.fireOnDisplayRefresh) or [Fire Periodically](vuo-node://vuo.time.firePeriodically2) to drive your composition (rather than, say, mouse or keyboard events).

If you change the quality or change the resolution of the images being sent, the stream will pause for a few seconds and then resume.  (This is because RTMP doesn't support dynamic resolution or bitrates, so this node has to disconnect from the server and reconnect to it, which takes a few seconds.)

RTMP only supports opaque images; if you feed this node a semitransparent image, it will become opaque (composited on a black background).
