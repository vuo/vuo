Outputs the smallest value received by this node so far.

This node keeps track of all the values received into its `Value` port, and outputs the one that is less than or equal to all the others. When an event hits the `Reset` port, the node forgets its minimum value and starts over with the current `Value` as the minimum.

Example: 

   - `Value` receives 1. `Minimum` becomes 1.
   - `Value` receives 4. `Minimum` stays at 1.
   - `Value` receives -3. `Minimum` becomes -3.

For point types, this node compares components left-to-right, stopping when a minimum component is found.  For example, (0,1) is considered less than (1,0).
