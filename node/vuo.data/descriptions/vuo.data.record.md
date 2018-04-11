Records a series of values to a file and plays them back.

This node is useful for recording live interactions, then making a high-quality offline render via File > Export > Movie….

When `Mode` is "Record":

   - `Record Value` — When an event hits this port, the data is appended to the recording and passed through to the output port (immediately if the event also hits the `Time` port, otherwise with the next `Time` event).
   - `Time` — A time, such as the output from the `Requested Frame` port of a graphics window node. Each time value should be greater than the previous one. The time is recorded along with the data.
   - `URL` — Where to save the recording. See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
   - `Overwrite URL` – If true, a file already existing at the `URL` will be overwritten with the new file.
   - `Finalize` — When an event hits this port, data stops being appended to the recording, and the file becomes ready to open by Vuo or another application. This port can be used to stop the current recording and start a new one.

Changing the `URL` while recording, changing the `Mode` from "Record" to "Playback", or stopping the composition has the same effect as `Finalize`.

When `Mode` is "Playback":

   - `Time` — The time of the the data to play back from the recording. If the time doesn't exactly match one of the recorded times, this node rounds down to the nearest recorded time. It only outputs an event if the rounded time is different from the previously played time. If the time is before the first recorded value, this node doesn't output an event. If the time is after the last recorded value, the last recorded value is played.
   - `URL` — The recording to use.

The `Record Value`, `Overwrite URL`, and `Finalize` ports are ignored while `Mode` is "Playback".

To create a high-quality offline render, use this node within an Image Generator protocol composition and connect the node's `Mode` port to a published `offlineRender` input port. This will set the mode to "Record" when the composition is run normally and to "Playback" when a movie is being exported (e.g., via the File > Export > Movie… menu option).

Some data types — Images, Video Frames, Trees, Meshes, Scene Objects, Layers, Shaders, and Windows — cannot currently be recorded.
