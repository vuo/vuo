Only lets a MIDI controller pass through if it matches all conditions.

   - `controller` — An event into this port is blocked unless the port's value meets all of the criteria specified by the other input ports.
   - `channelMin`, `channelMax` — The range of accepted channels. For all possible channels, use 1 to 16. 
   - `controllerNumberMin`, `controllerNumberMax` — The range of accepted controller numbers. For all possible controller numbers, use 0 to 127. 
   - `valueMin`, `valueMax` — The range of accepted controller values. For all possible controller values, use 0 to 127. 
