Only lets a MIDI controller pass through if its channel and controller number match.

   - `Controller` — An event into this port is blocked unless the port's value matches the specified channel and controller number.
   - `Channel` — Which channel to accept.
   - `Controller Number` — Which controller number to accept.

See also the `Filter and Smooth Controller` node, for a convenient way to scale and smooth MIDI controller values.
