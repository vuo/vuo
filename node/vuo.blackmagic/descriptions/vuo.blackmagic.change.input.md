Changes the connection and video mode on a Blackmagic video input device description.

Connect this node to `Receive Blackmagic Video` to dynamically choose a device.

   - `Device` — An input device description, typically from the `List Blackmagic Devices` node.
   - `Connection` — Which physical input source to use.
   - `Video Mode` — The resolution and framerate to use.  Not all modes are supported by all Blackmagic devices.
   - `Deinterlacing` — How to convert interlaced video to progressive video.  (See the [node set documentation](vuo-nodeset://vuo.blackmagic) for more info.)
