Outputs the frames of a movie in sequence. 

The `decodedImage` trigger port fires an event whenever it's time to display the next frame in the movie. It outputs that frame as an image. 

When the composition starts or this node is added to a running composition, the movie is paused at the beginning. It starts playing when the `play` input port receives an event. 

   - `movieURL` — The movie. When this input port receives an event, the movie is loaded (or reloaded, if `movieURL` has not changed), and the playhead goes back to the beginning of the movie. If the movie was playing, it continues playing. If it was paused, it stays paused. 
   - `play` — When this input port receives an event, the movie starts playing. If it was already playing, it continues. 
   - `loop` — What to do when the movie reaches the end. 
      - "Loop" makes the movie go back to the beginning (or, if playing in reverse, go back to the end). 
      - "None" makes this node stop firing events when the movie reaches the end (or, if playing in reverse, the beginning). 
