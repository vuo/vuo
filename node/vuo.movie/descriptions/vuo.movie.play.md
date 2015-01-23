Outputs the frames of a movie in sequence. 

The `decodedImage` trigger port fires an event whenever it's time to display the next frame in the movie. It outputs that frame as an image. 

When the composition starts or this node is added to a running composition, the movie is paused at `setTime` seconds from the beginning. It starts playing when the `play` input port receives an event. 

   - `movieURL` — The movie to play. When this port receives an event, the movie is loaded (or reloaded, if `movieURL` has not changed), and the playhead goes to the time specified by the `setTime` input port. If the movie was playing, it continues playing. If it was paused, it stays paused. 
   - `play` — When this port receives an event, the movie starts playing from the current playhead time. If it was already playing, it continues. 
   - `pause` — When this port receives an event, the movie stops playing and keeps the playhead at the current time. 
   - `setTime` — The time in the movie where the playhead should start, in seconds. When this port receives an event, the playhead goes to that time. If the movie was playing, it continues playing. If it was paused, it stays paused. 
   - `playbackRate` — The speed at which the movie plays. At 1, it plays at normal speed. At 2, it plays at double speed. At 0, the movie is paused. At -1, it plays backwards at normal speed. 
   - `loop` — What to do when the movie reaches the end. 
      - "Loop" makes the movie go back to the beginning (or, if playing in reverse, go back to the end). 
      - "Mirror" makes the movie play in reverse when it reaches the end, and play forward when it reaches the beginning. 
      - "None" makes this node stop firing events when the movie reaches the end (or, if playing in reverse, the beginning). 
