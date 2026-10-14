Fires an event for each item in the input list, and, depending on the result of the filter chain, either includes or excludes each item from the output list.

This node is useful for reducing a list to just certain items.  For example, you could use this node to take a list of audio input devices and exclude those that have a name containing certain text.

   - `Start Filtering` — When it receives an event, clears the node's internal list and causes the first in a series of events to fire from `Item`.
   - `Item` — Sequentially fires an event for each item in the `Start Filtering` input list.  Use this output to execute a chain of nodes that decides whether to keep each list item.
   - `Keep Item` — This port needs to receive each event fired by the `Item` port.  When it receives an event, if the value is `True`, it appends the current `Item` to the node's internal list, and it causes either `Item` or `Finished Filtering` to fire (depending on whether there are more list items to filter).
   - `Finished Filtering` — Fires an event containing the final list when filtering is complete.

If the `Start Filtering` port's value is an empty list when it receives an event, no events are fired from `Item`. Instead, `Finished Filtering` immediately fires an event with an empty list.

If the `Start Filtering` port receives an event while the node is still filtering the list for the previous `Start Filtering` event, then the later `Start Filtering` event is ignored (dropped). To avoid having events ignored, you can modify your composition to limit the rate at which events hit this node.
