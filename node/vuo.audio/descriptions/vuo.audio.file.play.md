Outputs a stream of audio from a file. 

If you drag an audio file from your computer onto the composition, an instance of this node will be created with the `URL` port set up.

The `Decoded Channels` trigger port fires an event whenever it's time to play the next sample buffer in the audio file.  It outputs a list, containing the next sample buffer for each channel in the audio file.

When the composition starts or this node is added to a running composition, the audio file is paused at `Set Time` seconds from the beginning. It starts playing when the `Play` input port receives an event.

   - `URL` — The audio file to play. When this port receives an event, the audio file is loaded (or reloaded, if `URL` has not changed), and the playhead goes to the time specified by the `Set Time` input port. If the audio file was playing, it continues playing. If it was paused, it stays paused.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
   - `Play` — When this port receives an event, the audio file starts playing from the current playhead time. If it was already playing, it continues.
   - `Pause` — When this port receives an event, the audio file stops playing and keeps the playhead at the current time.
   - `Set Time` — The time in the audio file where the playhead should start, in seconds. When this port receives an event, the playhead goes to that time. If the audio file was playing, it continues playing. If it was paused, it stays paused.
   - `Finished Playback` — Fires when playback has reached the end of the audio file.  (It does not fire when pausing the audio, or when there is an error decoding the audio.)
