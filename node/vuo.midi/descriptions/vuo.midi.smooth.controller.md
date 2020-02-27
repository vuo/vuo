Only lets a MIDI controller pass through if its channel and controller number match.

This node is similar to [Filter Controller](vuo-node://vuo.midi.filter.controller), but it also scales the MIDI CC value (0 to 127) to the real number range you specify, and smooths the value over time.

   - `Time` — The time at which to calculate the smoothed value.
   - `Controller` — An event into this port is blocked unless the port's value matches the specified channel and controller number.
   - `Channel` — Which channel to accept.
   - `Controller Number` — Which controller number to accept.
   - `Smoothing` — The range to scale the controller value to, the initial value, and how long the smoothing should take.
   - `Reset` — Outputs the initial value (specified by the `Smoothing` port).
