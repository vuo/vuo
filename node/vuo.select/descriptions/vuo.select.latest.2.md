Routes the data from whichever input port received an event to the output port.

This node is useful for coalescing data that comes from different data sources. It combines multiple streams of data into a single stream. This is useful when you have multiple nodes that should all send their data into a single input port on another node.

When an event comes in through one of the `option` input ports, that port's data and the event are passed on through the output port.

When an event comes in through all of the `option` input ports, or through the refresh port and none of the `option` input ports, then the `option1` port's data and the event are passed on through the output port.
