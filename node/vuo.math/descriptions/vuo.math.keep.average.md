Outputs the average of the values received by this node so far.

This node keeps track of all the values received into its `Value` port, and outputs their average (mean). Each time the node receives an event, the current `Value` is added to the set of values to be averaged.

When an event hits the `Reset` port, the node clears its set of values to be averaged and starts over with the current `Value` as the average.

For point types, this node averages each component of the value.
