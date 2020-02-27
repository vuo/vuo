Outputs a stream of data from a serial device.

This node can be used to allow (for example) sensors connected to an Arduino's analog inputs to control this composition.

   - `Device` — The serial device to receive from.  By default, this chooses the first available device.  Using the input editor, you can choose a specific device by serial number.  Or you can use [List Serial Devices](vuo-node://vuo.serial.listDevices) to allow the composition user to choose between their available devices.  Or you can use the [Specify Serial Device by Name](vuo-node://vuo.serial.make.name)/[URL](vuo-node://vuo.serial.make.url) nodes to choose a device by name or path.
   - `Received Data` — Fires an event each time some data is received.  When receiving text, lines may be broken up into multiple packets.  Use the [Split Text Stream](vuo-node://vuo.text.split.stream) node to coalesce the packets into consistent chunks.
