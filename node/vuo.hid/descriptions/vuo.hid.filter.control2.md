Only lets a USB HID control pass through if its name matches.

   - `Control` — An event into this port is blocked unless the control's value matches the specified name.
   - `Name` — The control name to match.  Uses [case-sensitive wildcard matching](vuo-nodeset://vuo.text).

See also the [Filter and Scale Control](vuo-node://vuo.hid.scale.control2) node, for a convenient way to scale USB HID control values.
