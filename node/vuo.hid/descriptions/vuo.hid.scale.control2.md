Only lets a USB HID control pass through if its name matches.

This node is similar to [Filter Control](vuo-node://vuo.hid.filter.control2), but it also scales the control's value to the real number range you specify.

   - `Control` — An event into this port is blocked unless the control's value matches the specified name.
   - `Name` — The control name to match.  Uses [case-sensitive wildcard matching](vuo-nodeset://vuo.text).
   - `Minimum`, `Maximum` — The range to scale the control value to.
