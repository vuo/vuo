Outputs one frame of a movie. 

When the composition starts, or this node is added to a running composition, or the `movieURL` input port receives an event, this node prepares to access the movie. Each time the `movieURL` input port receives a subsequent event with the same URL, this node updates its information about the movie in case the movie file has changed. 

When the `frameTime` or refresh input port receives an event, this node outputs one frame of the movie, as an image. It chooses the frame that would be playing at `frameTime` seconds after the start of the movie. For example, if `frameTime` is 2.0 and there's a frame that starts at exactly 2.0 seconds, then that frame is chosen. If `frameTime` is 3.0 and the nearest frames start at 2.8 and 3.1 seconds, then the frame starting at 2.8 seconds is chosen. If `frameTime` is before the first frame, then the first frame is chosen. If `frameTime` is after the last frame, then the last frame is chosen. 

If the `frameTime` or refresh input port receives an event while the node is still preparing to access the movie, it waits until the movie is ready, then outputs the frame. 
