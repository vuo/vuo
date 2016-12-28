Routes the input list to the selected output port.

This node is useful for activating different parts of a composition at different times. It can block events to part of the composition unless certain conditions are met.

The `Which` port selects the `Option` output port to which the data will be routed. If `Which` is 1 (or less), it selects `Option 1`. If `Which` is 8 (or more), it selects `Option 8`.

Events that come in through the `In` port or `Which` port are passed on through the selected output port (and no other output port).

This node's `Which` port lets you select between options using an integer (numerical) value. If instead you want to select between two options using a boolean (true/false) value, you can use a `Select Output List (Boolean)` node.
