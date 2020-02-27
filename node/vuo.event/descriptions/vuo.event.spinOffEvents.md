When this node receives an event, it fires multiple events. 

This node is useful for performing the same (or similar) sequence of steps repeatedly.  It's simpler and more flexible than copying and pasting the same sequence of nodes multiple times into your composition — you can easily change the number of iterations and the sequence of steps performed for each iteration.

If you want to perform steps repeatedly when working with a list, instead of this node it might be easier to use the [Build List](vuo-node://vuo.list.build) or [Process List](vuo-node://vuo.list.process) node in the `vuo.list` node set.
