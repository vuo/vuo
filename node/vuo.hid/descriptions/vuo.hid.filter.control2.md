Only lets a HID control pass through if its name matches.

   - `Control` — An event into this port is blocked unless the control's value matches the specified name.
   - `Name` — A search term that matches the full text of the control name.  Use [case-sensitive wildcard matching](vuo-nodeset://vuo.text), for example `*Shift` to match controls whose name ends with `Shift`, such as `Keyboard LeftShift` and `Keyboard RightShift`.

See also the [Filter and Scale Control](vuo-node://vuo.hid.scale.control2) node, for a convenient way to scale HID control values.
