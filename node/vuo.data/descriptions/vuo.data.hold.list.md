When its refresh port receives an event, this node outputs its stored list.

Before the `New Value` port has received an event, this node stores the `Initial Value` port's list. Typically, you would want to set a list of constant values on the `Initial Value` port rather than connecting a cable.

When the `New Value` port receives an event, this node replaces its stored list with the `New Value` port's current list.

This node is useful in feedback loops and other situations where you want to store a list with one event and later output it with another event.
