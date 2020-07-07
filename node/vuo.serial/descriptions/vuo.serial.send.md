Sends data to a serial device.

This node can be used to enable this composition to (for example) control an Arduino's digital outputs.

   - `Send Data` — When this port receives an event, the data is sent to the device.
   - `Device` — The serial device to send to.  By default, this chooses the first available device.  Using the input editor, you can choose a specific device by serial number.  Or you can use [List Serial Devices](vuo-node://vuo.serial.listDevices) to allow the composition user to choose between their available devices.  Or you can use the [Specify Serial Device by Name](vuo-node://vuo.serial.make.name)/[URL](vuo-node://vuo.serial.make.url) nodes to choose a device by name or path.
