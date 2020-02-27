Outputs the video frames and audio samples of a movie in sequence. 

If you drag a movie file from your computer onto the composition, an instance of this node will be created with the `URL` port set up.

When the composition starts or this node is added to a running composition, the movie is paused at `Set Time` seconds from the beginning. It starts playing when the `Play` input port receives an event. 

   - `Play` — When this port receives an event, the movie starts playing from the current playhead time. If it was already playing, it continues. 
   - `Pause` — When this port receives an event, the movie stops playing and keeps the playhead at the current time. 
   - `Set Time` — The time in the movie where the playhead should start, in seconds. When this port receives an event, the playhead goes to that time and fires a `Decoded Video` event. If the movie was playing, it continues playing. If it was paused, it stays paused. (If `Set Time` receives an event but `Decoded Video` doesn't fire, consider setting the `Decoded Video` port's event throttling mode to Enqueue Events.)
   - `URL` — The movie to play. When this port receives an event, the movie is loaded (or reloaded, if `URL` has not changed), and the playhead goes to the time specified by the `Set Time` input port. If the movie was playing, it continues playing. If it was paused, it stays paused.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
   - `Loop` — What to do when the movie reaches the end. 
      - "Loop" makes the movie go back to the beginning (or, if playing in reverse, go back to the end). 
      - "Mirror" makes the movie play in reverse when it reaches the end, and play forward when it reaches the beginning. 
      - "None" makes this node stop firing events when the movie reaches the end (or, if playing in reverse, the beginning). 
   - `Playback Rate` — The speed at which the movie plays. At 1, it plays at normal speed. At 2, it plays at double speed. At 0, the movie is paused. At -1, it plays backwards at normal speed if the movie has frames before the current playhead time (try enabling `Loop`, or `Set Time` to the movie's duration in seconds).
   - `Optimization` – How video playback should be optimized for best performance, based on the order in which video frames will be played.
      - `Auto` lets this node choose one of the optimization types below (*forward* or *random*). The optimization type is inferred from the values of `Playback Rate` and `Loop` when the composition starts (or when this port's value changes to `Auto`).
      - "Forward" optimizes for playing video frames in order.
      - "Random" optimizes for playing video frames in reverse order (such as if `Playback Rate` is -1 or `Loop` is "Mirror").
   - `Decoded Video` — Fires an event whenever it's time to display the next frame in the movie, which is outputted along with the event.
   - `Decoded Audio` — Fires an event whenever it's time to play the next audio samples in the movie, which are outputted along with the event. The `Decoded Audio` port only fires events when the movie is playing forward at normal speed. When the `Playback Rate` is not 1, or when the `Loop` is "Mirror" and the movie is playing in reverse, the `Decoded Audio` port does not fire events.
   - `Finished Playback` — Fires when playback has reached the end (or beginning) of the movie file, including when playback is set to loop or mirror. (It does not fire when pausing the movie, or when there is an error decoding the movie.)

If this node is unable to play as fast as the requested `Playback Rate`, it plays back the movie at the fastest rate that it can. The maximum rate at which this node is able to play back a movie is influenced by the movie's resolution and encoding; your computer's disk speed, CPU speed, and GPU speed; and other applications that are running on your computer.

An alternative to this node that's more suitable for protocol compositions, since it doesn't fire events, is [Decode Movie Image](vuo-node://vuo.video.decodeImage).
