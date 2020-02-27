Routes the selected input port's event to the output port.

This node is useful for switching between different ways of controlling part of a composition. For example, the `Option` inputs could come from up to 8 different input devices, and the `Which` input port could choose which input device to use.

The `Which` port selects the `Option` input port that will be routed to the output port. If `Which` is 1 (or less), it selects `Option 1`. If `Which` is 8 (or more), it selects `Option 8`.

Events that come in through the currently selected `Option` port are passed on through the output port. Events that come in through the other `Option` ports are blocked.

This node's `Which` port lets you select between options using an integer (numerical) value. If instead you want to select between two options using a boolean (true/false) value, you can use a [Select Event Input (Boolean)](vuo-node://vuo.select.in.boolean.event) node.
