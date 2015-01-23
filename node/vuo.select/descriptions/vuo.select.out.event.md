Routes an event to the selected output port.

This node is useful for activating different parts of a composition at different times. It can block events to part of the composition unless certain conditions are met.

The `which` port selects the `option` output port to which incoming events will be routed. If `which` is <i>false</i>, it selects `falseOption`. If `which` is <i>true</i>, it selects `trueOption`.

Events that come in through the `in` port or the refresh port are passed on through the selected output port.

There are two different classes of Select nodes for input and output. The first uses the values 1 and 2 to control how the node functions, while the second uses Boolean true or false values. The first set uses a class name beginning with `vuo.select.in.2` or `vuo.select.out.2` while the second set uses a class name beginning with `vuo.select.in` or `vuo.select.out`.
