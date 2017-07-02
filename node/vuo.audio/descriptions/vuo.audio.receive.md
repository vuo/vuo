Outputs a stream of audio from an input device. 

This node can be used to allow audio from a microphone, line input, or another source to control this composition.

   - `Device` — The device to receive from. If no device is given, then the system default audio input device is used.
   - `Received Channels` — Fires an event each time an audio sample buffer is received from the device.  Each item in the list corresponds to one audio channel (item 1 with channel 1, item 2 with channel 2, etc.).  The audio samples for each channel are typically numbers between -1 and 1.
