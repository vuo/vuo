Outputs the video frames and audio samples of a movie in sequence. 

If you drag a movie file from your computer onto the composition, an instance of this node will be created with the `URL` port set up.

When the composition starts or this node is added to a running composition, the movie is paused at `Set Time` seconds from the beginning. It starts playing when the `Play` input port receives an event. 

   - `URL` — The movie to play. When this port receives an event, the movie is loaded (or reloaded, if `URL` has not changed), and the playhead goes to the time specified by the `Set Time` input port. If the movie was playing, it continues playing. If it was paused, it stays paused. 
   - `Play` — When this port receives an event, the movie starts playing from the current playhead time. If it was already playing, it continues. 
   - `Pause` — When this port receives an event, the movie stops playing and keeps the playhead at the current time. 
   - `Set Time` — The time in the movie where the playhead should start, in seconds. When this port receives an event, the playhead goes to that time, and fires a `Decoded Video` event. If the movie was playing, it continues playing. If it was paused, it stays paused. (If `Set Time` receives an event but `Decoded Video` doesn't fire, consider setting the `Decoded Video` port's event throttling mode to Enqueue Events.)
   - `Playback Rate` — The speed at which the movie plays. At 1, it plays at normal speed. At 2, it plays at double speed. At 0, the movie is paused. At -1, it plays backwards at normal speed if the movie has frames before the current playhead time (try enabling `Loop`, or `Set Time` to the movie's duration in seconds).
   - `Loop` — What to do when the movie reaches the end. 
      - "Loop" makes the movie go back to the beginning (or, if playing in reverse, go back to the end). 
      - "Mirror" makes the movie play in reverse when it reaches the end, and play forward when it reaches the beginning. 
      - "None" makes this node stop firing events when the movie reaches the end (or, if playing in reverse, the beginning). 
   - `Decoded Video` — Fires an event whenever it's time to display the next frame in the movie, which is outputted along with the event.  `Decoded Frame` may be cast to either an image or a timestamp. 
   - `Decoded Audio` — Fires an event whenever it's time to play the next audio samples in the movie, which are outputted along with the event. 

The `Decoded Audio` port only fires events when the movie is playing forward at normal speed. When the `Playback Rate` is not 1, or when the `Loop` is "Mirror" and the movie is playing in reverse, the `Decoded Audio` port does not fire events.

The `Decoded Audio` port only fires events if the movie's audio format is supported by this node. The proprietary AAC and MP3 audio formats are not currently supported.
