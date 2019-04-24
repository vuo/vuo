Creates a description that can be used to look up a Blackmagic video input device.

Connect this node to `Receive Blackmagic Video` to dynamically choose a device.

   - `Name` — All or part of the device's name (as shown in the output of the `List Blackmagic Devices` node).  If more than one device matches the given name, one is arbitrarily chosen.
   - `Sub-device` — Some Blackmagic devices have multiple input ports of the same Connection type; this lets you choose between them.
   - `Connection` — Which physical input source to use.
   - `Video Mode` — The resolution and framerate to use.  Not all modes are supported by all Blackmagic devices.
   - `Deinterlacing` — How to convert interlaced video to progressive video.  (See the [node set documentation](vuo-nodeset://vuo.blackmagic) for more info.)
