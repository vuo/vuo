Routes the selected input port's data to the output port.

This node is useful for choosing between different data. The `option` inputs can come from different data sources, and the `which` input port can pick one of them to use.

The `which` port selects the `option` input port that will be routed to the output port. If `which` is <i>false</i>, it selects `falseOption`. If `which` is <i>true</i>, it selects `trueOption`.

Events that come in through the currently selected `option` port or the refresh port are passed on through the output port. Any other events are blocked.

There are two different classes of Select nodes for input and output. The first uses the values 1 and 2 to control how the node functions, while the second uses Boolean true or false values. The first set uses a class name beginning with `vuo.select.in.2` or `vuo.select.out.2` while the second set uses a class name beginning with `vuo.select.in` or `vuo.select.out`.
