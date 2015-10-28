Outputs the smallest value received by this node so far.

This node keeps track of all the values received into its `Value` port, and outputs the one that is less than or equal to all the others. When an event hits the `Reset` port, the node forgets its minimum value and starts over with the current `Value` as the minimum.

Example: 

   - `Value` receives 1. `Min` becomes 1.
   - `Value` receives 4. `Min` stays at 1.
   - `Value` receives -3. `Min` becomes -3.
