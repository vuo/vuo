Selects a USB HID device by matching its name.

Connect this node to `Receive HID Controls` to dynamically choose a USB HID device to use.

   - `Name` — All or part of the device’s name (the device's USB manufacturer, model, and HID class).  If more than one device matches the given name, the one with the lowest location in the USB bus is chosen.  Case-sensitive.
