Outputs one frame of a movie. 

When this node receives an event to its `frameTime` input port, it outputs the movie frame that would be playing at that time. It outputs that frame as an image. 

This node chooses the frame that would be playing at `frameTime` seconds after the start of the movie. For example, if `frameTime` is 2.0 and there's a frame that starts at exactly 2.0 seconds, then that frame is chosen. If `frameTime` is 3.0 and the nearest frames start at 2.8 and 3.1 seconds, then the frame starting at 2.8 seconds is chosen. 

   - `movieURL` — The movie to use. When this port receives an event, the movie is loaded (or reloaded, if `movieURL` has not changed). 
   - `frameTime` — The time of the frame to choose, in seconds. If the time is before the first frame, then the first frame is chosen. If the time is after the last frame, then the last frame is chosen. 
