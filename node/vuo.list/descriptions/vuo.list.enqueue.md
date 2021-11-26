Keeps track of a list of items.

When the composition starts or this node is added to a running composition, the list is empty.

   - `Add Item` — When this port receives an event, the item is added to the list if there's room.
   - `Max Item Count` — The maximum number of items in the list. If this is 0, the node always outputs an empty list. If this is `Auto`, the list will be unlimited.
   - `Discard when Full` — Which end of the list to discard from if the list exceeds `Max Item Count`.
      - `Oldest` — The oldest items are discarded to make room for the newest items.
      - `Newest` — New items are not added to the list.
   - `Clear List` — When this port receives an event, all items are removed from the list.
   - `List` — In order from oldest (first item) to newest (last item).

If an event hits both `Add Item` and `Clear List`, then the list is cleared before the item is added.

Thanks to [Martinus Magneson](https://vuo.org/user/3272) for the idea of the `Discard when Full` port.
