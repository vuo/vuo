Outputs a list of the MIDI notes that are currently on.

This node simulates a polyphonic MIDI keyboard — one that can play multiple notes simultaneously. As this node receives MIDI note on and note off events (as if keys on the keyboard were being pressed and released), the node outputs the list of nodes that are currently on (simulating the notes that would currently be playing on the keyboard).

   - `Note` — The most recent note. This node tracks all notes received in this input port that match the specified channel and range of note numbers. If the note doesn't match, or if it doesn't change the list of nodes that are currently on, then the event is blocked.
   - `Channel` — Which channel to accept.
   - `Note Number Min`, `Note Number Max` — Which note numbers to accept. For all possible note numbers, use 0 to 127.
   - `Reset` — When an event hits this port, this node clears its history of notes tracked.
   - `Notes`, `Note Numbers`, `Velocities` — Information about the notes that are currently on.

If the no matching note has come into the `Note` port since the composition was started or since an event hit the `Reset` port, then this node outputs empty lists for `Notes`, `Note Numbers`, and `Velocities`.
