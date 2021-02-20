Adds an item to a list, discarding the oldest item from the list if needed.

This node stores the most recently added items in a list. The list is "first in, first out" — as new items are added, the oldest items are removed to make room.

   - `Add Item` — When this port receives an event, the given item is added to the list. If this makes the list exceed `Max Item Count`, the item added longest ago is removed from the list.
   - `Max Item Count` — The maximum number of items in the list. If this is 0, the node always outputs an empty list. If this is `Auto`, the list will be unlimited.
   - `Clear List` — When this port receives an event, all items are removed from the list.
   - `List` — The list of recent items, in order from oldest to newest.

If an event hits both `Add Item` and `Clear List`, then the list is cleared before the item is added.
