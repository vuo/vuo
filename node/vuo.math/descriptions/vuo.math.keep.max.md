Outputs the largest value received by this node so far.

This node keeps track of all the values received into its `Value` port, and outputs the one that is greater than or equal to all the others. When an event hits the `Reset` port, the node forgets its maximum value and starts over with the current `Value` as the maximum.

Example: 

   - `Value` receives 4. `Max` becomes 4.
   - `Value` receives 1. `Max` stays at 4.
   - `Value` receives -6. `Max` stays at 4.

For point types, this node compares components left-to-right, stopping when a maximum component is found.  For example, (1,0) is considered greater than (0,1).
