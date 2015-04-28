Outputs the input value. 

This node is useful for passing the same data to multiple input ports. For example, if you want to draw several objects at the same height, then, rather than setting a height of (say) 0.5 in several different nodes' input editors, you can set 0.5 in this node's input editor and connect its output port to all of the height input ports. Then, if you decide to change the height to 1.0, you only have to change it in one input editor. 

This node is also useful for adding data to an event. For example, if you want this node to output the text *hello* each time it receives an event, then you can connect this node's output port to a text input port, and edit this node's input port value to *hello*. Each time this node receives an event, it will output the event plus the data *hello*. 
