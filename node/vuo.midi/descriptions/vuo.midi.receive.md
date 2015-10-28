Fires events when MIDI messages are received from an input device.

This node can be used to allow a MIDI keyboard, controller, or sequencer to control this composition.

   - `Device` — The device to receive from. If no device is given, then the first available MIDI input device is used.
   - `Received Note` — Fires an event each time a note message is received from the device.
   - `Received Controller` — Fires an event each time a controller message is received from the device.
