Fires events when OSC messages are received from an input device.

This node can be used to allow an external controller or sequencer to send information to this composition.

   - `Device` — The device to receive from, or the UDP port number to listen on.
      - If `Auto`, the node automatically creates a device named `Vuo OSC Server`, chooses an available port, and listens on the local network.
      - Otherwise, the node will listen on the UDP port specified by the device, if it is available.  If your device isn't listed in the menu, select a specific UDP port using the [Specify OSC Input](vuo-node://vuo.osc.make.input) node.  Often OSC devices communicate on ports 8000 and 9000.  Ports numbers less than 1024 are reserved and require elevated privileges.
   - `Received Message` — Fires an event each time a message is received from the device.  If a message bundle is received, fires an event for each message in the bundle. 

This node advertises itself via Bonjour, so OSC clients (controllers) can easily find it.

If multiple "Receive OSC Messages" nodes in the same composition are set to `Auto`, they will share a single automatically chosen port.
