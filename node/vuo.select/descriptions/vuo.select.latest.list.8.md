Routes the list from whichever input port received an event to the output port.

This node is useful for coalescing data that comes from different data sources. It combines multiple streams of data into a single stream. This is useful when you have multiple nodes that should all send their data into a single input port on another node.

When an event comes in through one of the `Option` input ports, that port's data and the event are passed on through the output port.

When an event comes in through multiple `Option` input ports, then the data and event of the lowest-numbered port to receive an event are passed on through the output port.

When an event comes in through the refresh port and none of the `Option` input ports, then the `Option 1` port's data and the event are passed on through the output port.
