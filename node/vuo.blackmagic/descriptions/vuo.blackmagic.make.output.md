Creates a description that can be used to look up a Blackmagic video output device.

Connect this node to `Send Blackmagic Video` to dynamically choose a device.

   - `Name` — All or part of the device's name (as shown in the output of the `List Blackmagic Devices` node).  If more than one device matches the given name, one is arbitrarily chosen.
   - `Sub-device` — Some Blackmagic devices have multiple output ports of the same Connection type; this lets you choose between them.
   - `Connection` — Which physical output destination to use.
   - `Video Mode` — The resolution and framerate to use.  Not all modes are supported by all Blackmagic devices.
