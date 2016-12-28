Fires events when OSC messages are received from an input device.

This node can be used to allow an external controller or sequencer to send information to this composition.

   - `Device` — The UDP port number to listen on.  The node advertises the selected port via Bonjour, so the controller can easily find it.
      - When "Automatic" is selected, an available port will be chosen automatically.  If multiple nodes in the same composition are set to Automatic, they will share a single automatically-chosen port.
      - Otherwise, the node will listen on the UDP port specified by the device, if it is available.  If your device isn't listed in the menu, select a specific UDP port using the `Make OSC Input` node.  Often OSC devices communicate on ports 8000 and 9000.  Ports numbers less than 1024 are reserved and require elevated privileges.
   - `Received Message` — Fires an event each time a message is received from the device.  If a message bundle is received, fires an event for each message in the bundle. 
