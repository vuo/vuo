Sends OSC messages to the network.

   - `Device` — The device to send to.
      - If `Auto`, the node automatically creates a device named `Vuo OSC Client`, chooses an available port, and broadcasts to the local network.
   - `Send Messages` — When this port receives an event, the messages are sent to the network.  If the list contains multiple items, the OSC messages are sent as a bundle.

To send messages to a separate Vuo composition, start the sender composition, then, in the receiver composition, use the input editor to select the `Vuo OSC Client` device.

The node advertises itself via Bonjour, so OSC servers can easily find it.

If multiple "Send OSC Messages" nodes in the same composition are set to `Auto`, they will share a single automatically chosen port.
