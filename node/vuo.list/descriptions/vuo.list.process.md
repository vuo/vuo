Fires an event for each item in the input list, and collects the results into a new list.

This node is useful for performing the same (or similar) sequence of steps on each item in a list.  For example, you could use this node to take a list of images and apply an effect to each one.  This is simpler and more flexible than copying and pasting the same sequence of nodes multiple times into your composition — you can easily change the number of iterations and the sequence of steps performed for each iteration.

   - `Fire` — When it receives an event, clears the node's internal list and rapidly fires an event for each item in the input list.
   - `Process Item` — Sequentially fires an event for each item in the `Fire` input list.  Use this output to execute a chain of nodes many times.
   - `Processed Item` — Expects to receive the events fired by the `Process Item` port.  When it receives an event, it appends its data to the node's internal list.  Once it receives the final event fired by this node, the list is complete, and the node fires `Processed List`.
   - `Processed List` — Fires an event containing the final list when iteration is complete.

If the `Fire` port receives an event while the node is still iterating through the list for the previous `Fire` event, then the later `Fire` event is ignored (dropped).
