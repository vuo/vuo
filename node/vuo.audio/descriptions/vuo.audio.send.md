Sends audio channels to an audio output device, and fires events when the audio device is ready for more.

This node can be used to hear audio that was read from a movie or sound file, or synthesized within Vuo.

   - `device` — The device to send to. If no device is given, then the system default audio output device is used.
   - `sendChannels` — When this port receives an event, the audio sample buffers are sent to the device. Each item in the list corresponds to one audio channel (item 1 with channel 1, item 2 with channel 2, etc.). 
   - `requestedChannels` — Fires an event when the output device is ready for another audio buffer.  The event's data is the audio output device's timestamp — the number of seconds since output began.

For the best sound quality, this node needs to receive events into its `sendChannels` port at the same rate that `requestedChannels` is firing events. If it receives sample buffers too quickly, then some buffers may be combined. If it receives sample buffers too slowly, then there may be short gaps of silence.

The audio samples received in the `sendChannels` audio port should typically fall between -1 and 1. If the samples fall outside that range, then the sound from the audio device may be distorted. 
