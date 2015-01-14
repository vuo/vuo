Calculates the time elapsed since this node started keeping track. 

This node is useful for controlling animations. You can connect the `requestedFrame` output port of a node that displays a graphics window (such as `Render Scene to Window`) to the `time` input port of this node. Starting when an event hits this node's `start` input port, each time the `time` input port receives an event, the `elapsedTime` port outputs a time relative to the start time. For example, if the `time` input port is receiving events every 1/60 seconds, the `elapsedTime` port outputs a series of values such as 0, 1/60, 2/60, 3/60, ... You can use these times to calculate an object's position or other parameters in an animation. 

   - `time` — A time, such as the output from the `requestedFrame` port of a graphics window node. Each time value should be greater than the previous one. 
   - `start` — When this port receives an event, the node begins (or continues) letting `time` events through. 
   - `pause` — When this port receives an event, the node stops letting `time` events through. 
   - `reset` — When this port receives an event, the node resets its start time to 0. If the node was paused, it stays paused. 
   - `elapsedTime` — The difference between the current value of `time` and the value of `time` when `start` got its first event or `reset` most recently got an event, not counting any time elapsed while the node was paused. 
