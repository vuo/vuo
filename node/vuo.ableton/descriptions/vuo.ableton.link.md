Provides musical timing info from an Ableton Link group on the local network.

   - `Time` — Connect `Fire on Display Refresh` to this port.
   - `Set Tempo` — When this port receives an event, it sends a tempo change request (in beats per minute) to the entire local group.  Ableton Link supports tempos ranging from 20 to 999 BPM.
   - `Start Transport`, `Stop Transport` — When these ports receive an event, they send a message to start/stop transport-based playback to the entire local group.  Apps can interpret these signals differently, and may ignore them entirely.  This Vuo node's behavior depends on the `Allow Events` value.
   - `Allow Events` — When this port is set to `Always`, events from the `Time` port are allowed through to the `Beat`/`Beats`/`Measure`/`Measures` ports, regardless of whether transport-based playback is running.  When set to `When Transport Running`, events are only allowed through to those ports when the transport is running.  In the latter case, when a `Start Transport` message is received, the transport starts at the beginning of the next full measure.
   - `Beats per Measure` — This affects the `Measure`-related output ports.  It's similar to the "Global Quantization" setting in Ableton Live.
   - `Time Offset` — Changes this node's timing relative to the rest of the local group.  This can help compensate for graphics processing/output lag.
   - `Peers` — How many other devices or apps are currently part of this local group.  This port allows an event through when a peer joins or leaves, and when the first `Time` is received.
   - `Tempo` — Outputs the local group's current tempo in beats per minute (BPM).  This port allows an event through when any peer changes the tempo, and when the first `Time` is received.
   - `Started` — Allows an event through at the beginning of the first measure _after_ a peer requests to start transport-based playback.  (If transport playback was started before the Vuo composition begins running, this port doesn't allow an event through.)
   - `Stopped` — Allows an event through when a peer requests to stop transport-based playback.
   - `Beat` — Allows an event through on each beat.
   - `Beats` — Outputs a value for each fractional beat.  Increases by 1 for each quarter note, and increases fractionally between quarter notes.  Resets to 0 when a peer starts the transport.
   - `Measure` — Allows an event through at the beginning of each measure (as specified by `Beats per Measure`).
   - `Measures` — Outputs a value for each fractional measure.  Increases by 1 for each measure, and it increases fractionally between measures.  Resets to 0 when a peer starts the transport.  For example, if `Beats per Measure` is 4, 0 = start of first measure; 1 = start of second measure; 1.5 = start of 3rd beat of second measure.

See <https://ableton.com/link> for a list of compatible apps.
