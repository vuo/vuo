Fires an event for each item in the input list, and collects the results into a new list.

This node is useful for performing the same (or similar) sequence of steps on each item in a list.  For example, you could use this node to take a list of images and apply an effect to each one.  This is simpler and more flexible than copying and pasting the same sequence of nodes multiple times into your composition — you can easily change the number of iterations and the sequence of steps performed for each iteration.

   - `Start Processing` — When it receives an event, clears the node's internal list and causes the first in a series of events to fire from `Item`.
   - `Item` — Sequentially fires an event for each item in the `Start Processing` input list.  Use this output to execute a chain of nodes many times.
   - `Processed Item` — This port needs to receive each event fired by the `Item` port.  When it receives an event, it appends its data to the node's internal list, and it causes either `Item` or `Finished Processing` to fire (depending on whether there are more list items to process).
   - `Finished Processing` — Fires an event containing the final list when iteration is complete.

If the `Start Processing` port's value is an empty list when it receives an event, no events are fired from `Item`. Instead, `Finished Processing` immediately fires an event with an empty list.

If the `Start Processing` port receives an event while the node is still iterating through the list for the previous `Start Processing` event, then the later `Start Processing` event is ignored (dropped). To avoid having events ignored, you can modify your composition to limit the rate at which events hit this node.
