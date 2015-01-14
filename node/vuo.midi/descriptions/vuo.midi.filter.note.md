Only lets a MIDI note pass through if it matches all conditions.

   - `note` — An event into this port is blocked unless the port's value meets all of the criteria specified by the other input ports.
   - `includeNoteOn`, `includeNoteOff` — If *true*, accept Note On or Note Off messages. At least one of these ports must be *true* for this node to accept any MIDI notes. 
   - `channelMin`, `channelMax` — The range of accepted channels. For all possible channels, use 1 to 16. 
   - `velocityMin`, `velocityMax` — The range of accepted velocities. For all possible velocities, use 0 to 127. 
   - `noteNumberMin`, `noteNumberMax` — The range of accepted note numbers. For all possible note numbers, use 0 to 127. 
