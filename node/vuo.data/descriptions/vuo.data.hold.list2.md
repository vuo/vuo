When its `Update` port receives an event, this node outputs its stored list.

When the `Value` port receives an event, this node replaces its stored list with the `Value` port's current list.

This node is useful in feedback loops and other situations where you want to store a list with one event and later output it with another event.
