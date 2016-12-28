Calculates the time elapsed since this node started keeping track. 

This node is useful for controlling animations. You can connect the `Requested Frame` output port of a node that displays a graphics window (such as `Render Scene to Window`) to the `Time` input port of this node. Starting when an event hits this node's `Start` input port, each time the `Time` input port receives an event, the `Elapsed Time` port outputs a time relative to the start time. For example, if the `Time` input port is receiving events every 1/60 seconds, the `Elapsed Time` port outputs a series of values such as 0, 1/60, 2/60, 3/60, ... You can use these times to calculate an object's position or other parameters in an animation. 

   - `Time` — A time, such as the output from the `Requested Frame` port of a graphics window node. Each time value should be greater than the previous one. 
   - `Start` — When this port receives an event, the node begins (or continues) letting `Time` events through. 
   - `Pause` — When this port receives an event, the node stops letting `Time` events through. 
   - `Reset` — When this port receives an event, the node resets its start time to 0, lets the next `Time` event through, and then stops letting `Time` events through.
   - `Elapsed Time` — The difference between the current value of `Time` and the value of `Time` when `Start` got its first event or `Reset` most recently got an event, not counting any time elapsed while the node was paused. 
