Outputs a stream of control data from a USB or Bluetooth HID device.

This node can be used to allow (for example) joysticks, gamepads, and sensors to control this composition.

   - `Device` — The HID to receive from.  Using the input editor, you can choose a class of devices or a specific device.  Or you can use [List HIDs](vuo-node://vuo.hid.listDevices) to allow the composition user to choose between their available devices.  Or you can use [Specify HID by Name](vuo-node://vuo.hid.make.name) to choose a device by name.
   - `Exclusive` — If true, this node tries to gain exclusive access to the device, preventing the operating system and other applications from using it.  For example, if you connect multiple mice, all mice will control the system cursor.  But if you enable Exclusive mode for a particular mouse, that mouse will no longer control the system cursor.  To exclusively read from a keyboard, macOS requires `root` access.
   - `Received Control` — Fires an event each time some data is received.  Use the [Filter Control](vuo-node://vuo.hid.filter.control2) or [Filter and Scale Control](vuo-node://vuo.hid.scale.control2) node to narrow down the data to just the controls you're interested in.

If you plug in additional devices while the composition is running, this node will continue to use the same device it had originally selected until you change the `Device` input or restart the composition.
