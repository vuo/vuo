When its refresh port receives an event, this node outputs its stored value.

Before the `New Value` port has received an event, this node stores the `Initial Value` port's value. Typically, you would want to set a constant value on the `Initial Value` port rather than connecting a cable.

When the `New Value` port receives an event, this node replaces its stored value with the `New Value` port's current value.

This node is useful in feedback loops and other situations where you want to store a value with one event and later output it with another event.
