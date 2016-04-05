Sends data to a serial device.

This node can be used to enable this composition to (for example) control an Arduino's digital outputs.

   - `Device` — The serial device to send to.  By default, this chooses the first available device.  Using the input editor, you can choose a specific device by serial number.  Or you can use `List Serial Devices` to allow the composition user to choose between their available devices.  Or you can use the `Make Serial Device` nodes to choose a device by name or path.
   - `Send Data` — When this port receives an event, the data is sent to the device.
