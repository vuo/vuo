Fires multiple events, and collects the results into a list.

This node is useful for creating a list of items in which each item is built using the same (or similar) sequence of steps.  For example, you could use this node to create a list of ovals, with each oval built using the same sequence of nodes, but each oval having a different color.  This is simpler and more flexible than copying and pasting the same sequence of nodes multiple times into your composition — you can easily change the number of iterations and the sequence of steps performed for each iteration.

   - `Fire` — When it receives an event, clears the node's internal list and causes the first in a series of events to fire from `Build Item`.
   - `Build Item` — Sequentially fires an event, with numbers ranging from 1 to `Fire`.  Use the event from this port to execute a chain of nodes many times.  Optionally, use the data from this port to recognize which item is being built and do something a little different for each item.
   - `Built Item` — This port needs to receive each event fired by the `Build Item` port.  When it receives an event, it appends its data to the node's internal list, and it causes either `Build Item` or `Built List` to fire (depending on whether there are more list items to build).
   - `Built List` — Fires an event containing the final list when iteration is complete.

If the `Fire` port's value is not greater than 0 when it receives an event, no events are fired from `Build Item`. Instead, `Built List` immediately fires a event with an empty list.

If the `Fire` port receives an event while the node is still building the list for the previous `Fire` event, then the later `Fire` event is ignored (dropped).
