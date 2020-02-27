When its `Update` port receives an event, this node outputs its stored value.

When the `Value` port receives an event, this node replaces its stored value with the `Value` port's current value.

This node is useful in feedback loops and other situations where you want to store a value with one event and later output it with another event.
