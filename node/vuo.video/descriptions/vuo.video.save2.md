Saves images and audio to a movie file.

The movie file is created and saving starts when the `Save Video Frame` port receives its first frame. As the `Save Video Frame` port continues receiving frames, and the `Save Audio Frame` port optionally receives audio, these are appended to the movie file in real time. Saving continues until the `Finalize` port receives an event or the composition stops.

Certain characteristics of the movie are determined when saving starts, and can't be changed while saving is in progress. The width and height of the movie are determined by the first video frame received. Whether or not the movie has audio is determined within the first half second after the first video frame is received. If no audio is received during that time, then the movie won't have an audio track. The number of audio channels is determined by the first audio frame received.

   - `URL` — Where to save the movie.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
   - `Save Video Frame` — Appends a video frame (an image with a timestamp) to the movie file.
   - `Save Audio Frame` — Appends an audio frame (a list of audio samples with a timestamp) to the movie file.
   - `Finalize` — When this port receives an event, frames stop being appended to the movie file, and the file becomes ready to open by Vuo or another application. This port can be used to stop recording the current movie and start recording a new one.
   - `Overwrite URL` — If the file at `URL` already exists when this node wants to start saving to it, this port determines what should happen. If *true*, the existing file will be deleted and replaced with the new movie. If *false*, the existing file will be preserved, and the new movie won't be saved.
   - `Format` — Settings for the movie's encoding and quality.
