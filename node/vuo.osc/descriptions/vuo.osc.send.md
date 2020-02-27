Sends OSC messages to the network.

   - `Device` — The device to send to.  If no device is given, the node will automatically choose an available port and broadcast on the local network. If multiple "Send OSC Messages" nodes in the same composition are set to `Auto`, they will share a single automatically chosen port.
   - `Send Messages` — When this port receives an event, the messages are sent to the network.  If the list contains multiple items, the OSC messages are sent as a bundle.
