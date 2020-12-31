Routes the selected input port's event to the output port.

This node is useful for switching between different ways of controlling part of a composition. For example, the `Option` inputs could come from two different input devices, and the `Which` input port could choose which input device to use.

The `Which` port selects the `Option` input port that will be routed to the output port. If `Which` is <i>false</i>, it selects `False Option`. If `Which` is <i>true</i>, it selects `True Option`.

Events that come in through the currently selected `Option` port are passed on through the output port. Events that come in through the other `Option` port are blocked.

This node's `Which` port lets you select between options using a boolean (true/false) value. If instead you want to select between two options using an integer (numerical) value, you can use a [Select Event Input (2)](vuo-node://vuo.select.in.event2.2) or [Select Event Input (8)](vuo-node://vuo.select.in.event2.8) node.
