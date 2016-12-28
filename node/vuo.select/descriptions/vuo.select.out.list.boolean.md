Routes the input list to the selected output port.

This node is useful for activating different parts of a composition at different times. It can block events to part of the composition unless certain conditions are met.

The `Which` port selects the `Option` output port to which the data will be routed. If `Which` is <i>false</i>, it selects `False Option`. If `Which` is <i>true</i>, it selects `True Option`.

Events that come in through the `In` port or `Which` port are passed on through the selected output port (and no other output port).

This node's `Which` port lets you select between options using a boolean (true/false) value. If instead you want to select between two options using an integer (numerical) value, you can use a `Select Output List (2)` or `Select Output List (8)` node.
