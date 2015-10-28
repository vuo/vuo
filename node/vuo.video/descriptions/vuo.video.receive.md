Outputs frames of video from a connected device.

To begin receiving frames, send an event to the `Start` port.

If no input device is specified, the first available video input will be used.  Most of the time, this will the Macbook's iSight.

You can you use `List Video Devices` to see available input devices, and `Make Video Input` to choose a device by name.  If the `Device` input port is set, this node will not output video until a device matching the specified device is found.
