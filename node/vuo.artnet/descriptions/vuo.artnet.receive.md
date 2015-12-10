Outputs a stream of Art-Net DMX512 lighting data from the network.

This node can be used to allow a stage lighting system to control this composition.

   - `Device` — The device to receive from. If no device is given, then the Art-Net address Net 0, Sub-Net 0, Universe 0 is used.
   - `Received DMX` — Fires an event each time a frame of DMX512 lighting data is received — typically about 40 times per second.  Each item in the list corresponds to one lighting channel (item 1 with channel 1, item 2 with channel 2, etc.).  The value for each channel ranges from 0 to 255.
