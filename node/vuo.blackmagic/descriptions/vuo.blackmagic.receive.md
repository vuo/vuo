Outputs images from a Blackmagic Design video capture device.

If no input device is specified, the first available video input will be used.

You can you use `List Blackmagic Devices` to see available input devices, and `Make Blackmagic Input` to choose a device by name.  If a specific device is selected in the `Device` input port, this node will not output video until a matching device is found.

Thanks to [Stuart White of Control Freak Systems](https://vuo.org/user/1426) for commissioning the code this node is based on.
