Only lets a USB HID control pass through if its name matches.

   - `Control` — An event into this port is blocked unless the control's value matches the specified name.
   - `Name` — All or part of the control’s name.  Case-sensitive.

See also the [Filter and Scale Control](vuo-node://vuo.hid.scale.control) node, for a convenient way to scale USB HID control values.
