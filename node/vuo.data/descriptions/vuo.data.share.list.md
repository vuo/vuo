Outputs the input list.

This node is useful for passing the same list to multiple input ports. For example, if you want to input the list *A*, *B*, *C* into multiple nodes, rather than typing the same list into the drawer of each node, you can type the list into the drawer of this node and connect its output port to all of the desired input ports. Then, if you decide to change the list, you only have to change it in one drawer.

This node is also useful for adding data to an event. Each time this node receives an event, it will output the event plus the data from the input port.
