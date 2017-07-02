Outputs one frame of a video.

When this node receives an event to its `Frame Time` input port, it outputs the video frame that would be playing at that time.

This node chooses the frame that would be playing at `Frame Time` seconds after the start of the video. For example, if `Frame Time` is 2.0 and there's a frame that starts at exactly 2.0 seconds, then that frame is chosen. If `Frame Time` is 3.0 and the nearest frames start at 2.8 and 3.1 seconds, then the frame starting at 2.8 seconds is chosen.

   - `URL` — The video to use. When this port receives an event, the video is loaded (or reloaded, if `URL` has not changed).  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
   - `Frame Time` — The time of the frame to choose, in seconds. If the time is before the first frame, then the first frame is chosen. If the time is after the last frame, then the last frame is chosen.
   - `Loop` — What to do when the `Frame Time` is greater than the duration of the video.
      - "Loop" makes the movie go back to the beginning.
      - "Mirror" makes the movie play in reverse when it reaches the end, and play forward when it reaches the beginning.
      - "None" makes this node continue sending the last decoded frame until a valid time is provided.
