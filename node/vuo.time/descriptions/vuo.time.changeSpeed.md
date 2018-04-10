Makes time run faster, slower, or backwards.

This node is useful for dynamically changing how fast an animation happens.  Unlike using the `Multiply` node (which will cause time to jump if you change the speed multiplier), this node ensures time is continuous when changing the speed.

   - `Time` — A time, such as the output from the `Requested Frame` port of a graphics window node. Each time value should be greater than the previous one.
   - `Speed` — How fast time should move.  Speed 1 means the output time will move at the same speed as the input time.  Speed 2 is twice as fast.  Speed 0 is stopped.  Speed -1 is backwards at the same speed as the input time.
   - `Reset` — When this port receives an event, the node resets its time to 0 and continues running.

Thanks to [vjsatoshi](https://vuo.org/user/277) for developing the node this is based on.
