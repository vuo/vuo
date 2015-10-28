Provides an on/off switch that can be toggled, set to *true*, or set to *false*. 

   - `Toggle` — When this port receives an event, the node outputs *true* if the previous output was *false*, and *false* if the previous output was *true*. 
   - `Turn On` — When this port receives an event, the node outputs *true*. 
   - `Turn Off` — When this port receives an event, the node outputs *false*. 

This node starts out in an "off" state. The first time it's toggled (unless it's previously been turned on), it outputs *true*.

If the same event hits multiple ports, then the lowest port controls the output. `Turn Off` takes precedence over `Turn On`, and `Turn On` takes precedence over `Toggle`. 
