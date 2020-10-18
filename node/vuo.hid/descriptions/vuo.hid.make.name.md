Selects a HID by matching its name.

Connect this node to [Receive HID Controls](vuo-node://vuo.hid.receive) to dynamically choose a device to use.

   - `Name` — All or part of the device’s name (the device's manufacturer, model, and HID class).  If more than one device matches the given name, one is arbitrarily chosen.  Case-sensitive.
