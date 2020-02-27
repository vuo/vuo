Fires multiple events, and collects the results into a list.

This node is useful for creating a list of items in which each item is built using the same (or similar) sequence of steps.  For example, you could use this node to create a list of images, with each image built using the same sequence of nodes, but each image having a different color.  This is simpler and more flexible than copying and pasting the same sequence of nodes multiple times into your composition — you can easily change the number of iterations and the sequence of steps performed for each iteration.

   - `Start Building` — When it receives an event, clears the node's internal list and causes the first in a series of events to fire from `Index`.
   - `Index` — Sequentially fires an event, along with a number from 1 to the number of items in `Start Building`.  Use the event from this port to execute a chain of nodes many times.  Optionally, use the data from this port to recognize which item is being built and do something a little different for each item.
   - `Built Item` — This port needs to receive each event fired by the `Index` port.  When it receives an event, it appends its data to the node's internal list, and it causes either `Index` or `Finished Building` to fire (depending on whether there are more list items to build).
   - `Finished Building` — Fires an event containing the final list when iteration is complete.

If the `Start Building` port's value is not greater than 0 when it receives an event, no events are fired from `Index`. Instead, `Finished Building` immediately fires an event with an empty list.

If the `Start Building` port receives an event while the node is still building the list for the previous `Start Building` event, then the later `Start Building` event is ignored (dropped). To avoid having events ignored, you can modify your composition to limit the rate at which events hit this node.
