Routes the selected input port's list to the output port.

This node is useful for choosing between different data. The `Option` inputs can come from different data sources, and the `Which` input port can pick one of them to use.

The `Which` port selects the `Option` input port that will be routed to the output port. If `Which` is 1 (or less), it selects `Option 1`. If `Which` is 2 (or more), it selects `Option 2`.

Events that come in through the currently selected `Option` port are passed on through the output port. Events that come in through the other `Option` port are blocked.

This node's `Which` port lets you select between options using an integer (numerical) value. If instead you want to select between two options using a boolean (true/false) value, you can use a `Select Input List (Boolean)` node.
