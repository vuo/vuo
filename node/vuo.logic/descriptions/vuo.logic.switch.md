Provides an on/off switch that can be toggled, set to *true*, or set to *false*. 

   - `toggle` — When this port receives an event, the node outputs *true* if the previous output was *false*, and *false* if the previous output was *true*. 
   - `turnOn` — When this port receives an event, the node outputs *true*. 
   - `turnOff` — When this port receives an event, the node outputs *false*. 

If the `toggle` port receives an event before the node has output anything, the node outputs *true*. 

If the same event hits multiple ports, then the lowest port controls the output. `turnOff` takes precedence over `turnOn`, and `turnOn` takes precedence over `toggle`. 
