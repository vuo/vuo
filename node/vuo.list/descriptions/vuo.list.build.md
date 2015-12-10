Fires multiple events, and collects the results into a list.

This node is useful for creating a list of items in which each item is built using the same (or similar) sequence of steps.  For example, you could use this node to create a list of ovals, with each oval built using the same sequence of nodes, but each oval having a different color.  This is simpler and more flexible than copying and pasting the same sequence of nodes multiple times into your composition — you can easily change the number of iterations and the sequence of steps performed for each iteration.

   - `Fire` — When it receives an event, clears the node's internal list and rapidly fires multiple events.
   - `Build Item` — Sequentially fires an event, with numbers ranging from 1 to `Fire`.  Use the event from this port to execute a chain of nodes many times.  Optionally, use the data from this port to recognize which item is being built and do something a little different for each item.
   - `Built Item` — Expects to receive the events fired by the `Build Item` port.  When it receives an event, it appends its data to the node's internal list.  Once it receives the final event fired by this node, the list is complete, and the node fires `Built List`.
   - `Built List` — Fires an event containing the final list when iteration is complete.

If the `Fire` port receives an event while the node is still building the list for the previous `Fire` event, then the later `Fire` event is ignored (dropped).
