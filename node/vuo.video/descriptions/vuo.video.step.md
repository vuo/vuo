Advances through a video one frame at a time.

   - `URL` — The video to step through. When this port receives an event, the video is loaded (or reloaded, if `URL` has not changed).  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
   - `Go Forward` — Advances to the next chronological video frame.
   - `Go Backward` — Moves to the previous chronological video frame.
   - `Set Time` — Moves the playhead to the frame at the provided timestamp, in seconds.
   - `Loop` — What to do when the current frame time is greater than the duration of the video.
      - Loop: When `Next` reaches the end of a movie it returns to the start.  When `Previous` reaches the beginning of the movie it moves to the final frame.
      - None: When `Next` or `Previous` reaches the end or beginning (respectively) of a movie, any subsequent events to the port output the same video frame.
