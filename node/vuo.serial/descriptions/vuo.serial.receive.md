Outputs a stream of data from a serial device.

This node can be used to allow (for example) sensors connected to an Arduino's analog inputs to control this composition.

   - `Device` — The serial device to receive from.  By default, this chooses the first available device.  Using the input editor, you can choose a specific device by serial number.  Or you can use `List Serial Devices` to allow the composition user to choose between their available devices.  Or you can use the `Make Serial Device` nodes to choose a device by name or path.
   - `Received Data` — Fires an event each time some data is received.  When receiving text, lines may be broken up into multiple packets.  Use the `Split Text Stream` node to coalesce the packets into consistent chunks.
