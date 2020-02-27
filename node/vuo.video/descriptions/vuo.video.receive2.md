Outputs frames of video from a connected device.

If no input device is specified, the system default will be used.

You can you use [List Video Devices](vuo-node://vuo.video.listDevices) to see available input devices, and [Specify Video Input](vuo-node://vuo.video.make.input) to choose a device by name.  If the `Device` input port is set, this node will not output video until a device matching the specified device is found.

When `Width` and `Height` are set to `Auto`, this node will output frames at the resolution for which the device provides the best framerate.  Otherwise, this node configures the device to whichever of its supported resolutions is closest to `Width` and `Height`.
