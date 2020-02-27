Routes an event to the selected output port.

This node is useful for activating different parts of a composition at different times. It can block events to part of the composition unless certain conditions are met.

The `Which` port selects the `Option` output port to which incoming events will be routed. If `Which` is <i>false</i>, it selects `False Option`. If `Which` is <i>true</i>, it selects `True Option`.

Events that come in through the `In` port or `Which` port are passed on through the selected output port (and no other output port).

This node's `Which` port lets you select between options using a boolean (true/false) value. If instead you want to select between two options using an integer (numerical) value, you can use a [Select Event Output (2)](vuo-node://vuo.select.out.event.2) or [Select Event Output (8)](vuo-node://vuo.select.out.event.8) node.
