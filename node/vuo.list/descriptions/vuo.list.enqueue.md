Adds an item to a limited-size list, discarding the oldest item from the list if needed. 

This node stores the most recently added items in a list. The list is "first in, first out" — as new items are added, the oldest items are removed to make room. 

   - `maxItemCount` — The maximum number of items in the list. If this is 0, the node always outputs an empty list. 
   - `addItem` — When this port receives an event, the given item is added to the list. If this makes the list exceed `maxItemCount`, the item added longest ago is removed from the list. 
   - `clearList` — When this port receives an event, all items are removed from the list. 
   - `list` — The list of recent items, in order from oldest to newest. 

If an event hits both `addItem` and `clearList`, then the list is cleared before the item is added. 
