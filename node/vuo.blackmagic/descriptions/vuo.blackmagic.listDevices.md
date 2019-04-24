Outputs a list of all connected Blackmagic devices.

Since not all Blackmagic devices can automatically detect the current connection and video mode, this node lets you specify the default connection and video mode for all devices, and also lets you specify how to convert interlaced video to progressive video.  (See the [node set documentation](vuo-nodeset://vuo.blackmagic) for more info.)

This node fires an event when a device is connected or disconnected, and when the default connection or video mode is changed.
