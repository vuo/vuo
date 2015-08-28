Outputs a stream of audio from a file. 

The `decodedChannels` trigger port fires an event whenever it's time to play the next sample buffer in the audio file.  It outputs a list, containing the next sample buffer for each channel in the audio file.

When the composition starts or this node is added to a running composition, the audio file is paused at `setTime` seconds from the beginning. It starts playing when the `play` input port receives an event.

   - `url` — The audio file to play. When this port receives an event, the audio file is loaded (or reloaded, if `url` has not changed), and the playhead goes to the time specified by the `setTime` input port. If the audio file was playing, it continues playing. If it was paused, it stays paused.
   - `play` — When this port receives an event, the audio file starts playing from the current playhead time. If it was already playing, it continues.
   - `pause` — When this port receives an event, the audio file stops playing and keeps the playhead at the current time.
   - `setTime` — The time in the audio file where the playhead should start, in seconds. When this port receives an event, the playhead goes to that time. If the audio file was playing, it continues playing. If it was paused, it stays paused.
   - `finishedPlayback` — Fires when playback has reached the end of the audio file.  (It does not fire when pausing the audio, or when there is an error decoding the audio.)
