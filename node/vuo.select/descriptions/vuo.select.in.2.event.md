Routes the selected input port's event to the output port.

This node is useful for switching between different ways of controlling part of a composition. For example, the `option` inputs could come from two different input devices, and the `which` input port could choose which input device to use.

The `which` port selects the `option` input port that will be routed to the output port. If `which` is 1 (or less), it selects `option1`. If `which` is 2 (or more), it selects `option2`.

Events that come in through the currently selected `option` port or the refresh port are passed on through the output port. Any other events are blocked.

There are two different classes of Select nodes for input and output. The first uses the values 1 and 2 to control how the node functions, while the second uses Boolean true or false values. The first set uses a class name beginning with `vuo.select.in.2` or `vuo.select.out.2` while the second set uses a class name beginning with `vuo.select.in` or `vuo.select.out`.
