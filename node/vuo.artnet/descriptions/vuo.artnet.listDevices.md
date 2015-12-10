Gives a list of Art-Net devices on the local network that are capable of sending or receiving DMX512.

This node fires an event with an updated list of Art-Net devices whenever it sees a change in the devices available — specifically, when the composition starts or this node is added to a running composition when Art-Net devices are already available; when a new Art-Net device becomes available; when an Art-Net device becomes unavailable; or when an Art-Net device's name, IP, Ethernet address, or Port-Address changes.

If a single device has multiple ports, each port is a separate item in the output lists.
