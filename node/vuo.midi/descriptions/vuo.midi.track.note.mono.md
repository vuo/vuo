Chooses a single MIDI note of the notes that are currently on, based on a note priority rule.

This node simulates a monophonic MIDI keyboard — one that can only play one note at a time. As this node receives MIDI note on and note off events (as if keys on the keyboard were being pressed and released), the node chooses a single note to output (simulating the note that would currently be playing on the keyboard)

   - `Note` — The most recent note. This node tracks all notes received in this input port that match the specified channel and range of note numbers. If the note doesn't match, or if it doesn't change the chosen note, then the event is blocked.
   - `Channel` — Which channel to accept.
   - `Note Number Min`, `Note Number Max` — Which note numbers to accept. For all possible note numbers, use 0 to 127.
   - `Note Priority` — The rule for determining which note is chosen among the notes that are currently on.
      - "First Note" — The note that has been continuously on for the longest time is chosen.
      - "Last Note" — The note that most recently came on is chosen.
      - "Lowest Note" — The lowest-pitched note is chosen.
      - "Highest Note" — The highest-pitched note is chosen.
   - `Reset` — When an event hits this port, this node clears its history of notes tracked.
   - `Note Number`, `Velocity` — Information about the chosen note.

If the no matching note has come into the `Note` port since the composition was started or since an event hit the `Reset` port, then this node outputs 0 for `Velocity`.
