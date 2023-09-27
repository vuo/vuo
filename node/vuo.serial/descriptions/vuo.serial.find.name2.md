Given a list of serial devices, finds the devices whose names match a specified name.

   - `Serial Devices` — A list of serial devices, such as that from the [List Serial Devices](vuo-node://vuo.serial.listDevices) node.
   - `Name` — A search term that matches the full text of the serial device name.  Use [case-sensitive wildcard matching](vuo-nodeset://vuo.text), for example `Arduino*` to match devices whose name begins with `Arduino`.
