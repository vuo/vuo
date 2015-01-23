When its refresh port receives an event, this node outputs its stored value.

Before the `newValue` port has received an event, this node stores the `initialValue` port's value. Typically, you would want to set a constant value on the `initialValue` port rather than connecting a cable.

When the `newValue` port receives an event, this node replaces its stored value with the `newValue` port's current value.

This node is useful in feedback loops and other situations where you want to store a value with one event and later output it with another event.
