Gives a list of OSC devices on the local network that are capable of sending or receiving data.

This node fires an event with an updated list of OSC devices whenever it sees a change in the devices available — specifically, when the composition starts or this node is added to a running composition when OSC devices are already available; when a new OSC device becomes available; when an OSC device becomes unavailable; or when an OSC device's name, IP, or port changes.

For a device or application to appear in this list, it must advertise its OSC service using [Zeroconf](http://www.zeroconf.org)/<a href="https://en.wikipedia.org/wiki/Bonjour_(software)">Bonjour</a>/mDNS.  (Vuo also supports devices and applications that don't advertise via Zeroconf/Bonjour/mDNS — use [Specify OSC Input](vuo-node://vuo.osc.make.input) or [Specify OSC Output](vuo-node://vuo.osc.make.output) in that case.)

If a single device has multiple IP addresses or ports, each is a separate item in the output lists.
