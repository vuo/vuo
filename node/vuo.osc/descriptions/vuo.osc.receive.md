Fires events when OSC messages are received from an input device.

This node can be used to allow an external controller or sequencer to send information to this composition.

   - `udpPort` — The UDP port number to listen on.  The node advertises the selected port via Bonjour, so the controller can easily find it.
      - If the port is 0, an available port will be chosen automatically.  If multiple nodes in the same composition use port 0, they will share a single automatically-chosen port.
      - If the port is nonzero, the node will listen on that specific port, if it is available.  Often OSC devices communicate on ports 8000 and 9000.  Ports numbers less than 1024 are reserved and require elevated privileges.
   - `receivedMessage` — Fires an event each time a message is received from the device.  If a message bundle is received, fires an event for each message in the bundle. 
