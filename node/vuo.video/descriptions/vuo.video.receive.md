Outputs frames of video from a connected device.

To begin receiving frames, send an event to the `Start` port.

If no input device is specified, the first available video input will be used.

You can you use [List Video Devices](vuo-node://vuo.video.listDevices) to see available input devices, and [Make Video Input](vuo-node://vuo.video.make.input) to choose a device by name.  If the `Device` input port is set, this node will not output video until a device matching the specified device is found.

When `Width` and `Height` are set to `Auto`, this node will output frames at the device's native resolution.  Some cameras output frames at a higher framerate if you specify a resolution lower than the native resolution.  For example, the Logitech C910 camera has a native resolution of 2592x1944 pixels, but it can only output about 10 frames per second at that resolution.  Lowering the resolution to 1920x1080 enables it to output 30 frames per second.
