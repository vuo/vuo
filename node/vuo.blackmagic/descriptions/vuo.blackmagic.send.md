Sends images to a Blackmagic Design video output device.

If no output device is specified, the first available video output will be used.

You can you use `List Blackmagic Devices` to see available output devices, and `Make Blackmagic Output` to choose a device by name.  If a specific device is selected in the `Device` input port, this node will not send video until a matching device is found.

Thanks to [Stuart White of Control Freak Systems](https://vuo.org/user/1426) for commissioning the code this node is based on.
