Only lets a USB HID control pass through if its name matches.

This node is similar to `Filter Control`, but it also scales the control's value to the real number range you specify.

   - `Control` — An event into this port is blocked unless the control's value matches the specified name.
   - `Name` — All or part of the control’s name.  Case-sensitive.
   - `Minimum`, `Maximum` — The range to scale the control value to.
