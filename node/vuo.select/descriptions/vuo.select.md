Nodes for routing data and events through a composition.

To choose between different parts of the composition to execute, use a `Select Output` node. 

To choose between different data and event sources, use a `Select Input` node. 

To coalesce events from different sources, use a `Select Latest` node. 

To block events from one source while letting events from another source pass through, use a [Hold Value](vuo-node://vuo.data.hold2) node from the [vuo.data](vuo-nodeset://vuo.data) node set. 

For further explanation of these nodes, see the [Vuo Manual](https://doc.vuo.org/latest/manual/route-dataevents-through-the-composition.xhtml).
