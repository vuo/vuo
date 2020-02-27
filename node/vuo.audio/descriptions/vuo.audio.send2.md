Sends audio channels to an audio output device.

You can use this node to hear audio that was read from a movie or sound file or synthesized within Vuo.

   - `Device` — The device to send to. If no device is given, then the system default audio output device is used.
   - `Send Channels` — When this port receives an event, the audio sample buffers are sent to the device. Each item in the list corresponds to one audio channel (item 1 with channel 1, item 2 with channel 2, etc.).

For the best sound quality, this node needs to receive events into its `Send Channels` port at the same rate that [Fire at Audio Rate](vuo-node://vuo.audio.fireAtBufferRate) is firing events. If it receives sample buffers too quickly, then some buffers may be combined. If it receives sample buffers too slowly, then there may be short gaps of silence.

If the `Send Channels` port receives fewer list items than the number of channels in the audio device, the last item in the list is duplicated to fill the remaining channels. For example, if mono audio (a single list item) is provided for a stereo audio device, this node sends the audio to both stereo channels.

The audio samples received in the `Send Channels` audio port should typically fall between -1 and 1. If the samples fall outside that range, then the sound from the audio device may be distorted.
