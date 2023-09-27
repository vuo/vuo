Given a list of HID devices, finds the devices whose names match a specified name.

   - `HIDs` — A list of devices, such as that from the [List HIDs](vuo-node://vuo.hid.listDevices) node.
   - `Name` — A search term that matches the full text of the HID device name.  Use [case-sensitive wildcard matching](vuo-nodeset://vuo.text), for example `*Mouse*` to match devices whose name contains `Mouse`.
