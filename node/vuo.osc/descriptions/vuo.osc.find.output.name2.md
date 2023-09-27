Given a list of OSC output devices, finds the devices whose names match a specified name.

   - `Devices` — A list of OSC output devices, such as that from the [List OSC Devices](vuo-node://vuo.osc.listDevices) node.
   - `Name` — A search term that matches the full text of the OSC device name.  Use [case-sensitive wildcard matching](vuo-nodeset://vuo.text), for example `*TouchOSC*` to match devices whose name contains `TouchOSC`.
