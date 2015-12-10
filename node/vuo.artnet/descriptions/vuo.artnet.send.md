Sends Art-Net DMX512 lighting data to the network.

This node can be used to enable this composition to control stage lights.

   - `Device` — The device to send to. If no device is given, the node will broadcast on the local network to Art-Net address Net 0, Sub-Net 0, Universe 0.
   - `Send DMX` — When this port receives an event, the list of lighting values are sent to the device. Each item in the list corresponds to one lighting channel (item 1 with channel 1, item 2 with channel 2, etc.). Values are clamped to between 0 and 255.
