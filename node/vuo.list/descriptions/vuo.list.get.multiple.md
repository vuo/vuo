Selects multiple items from a list.

`Positions` are the numbers of the items to pick (1 for the first item, 2 for the second item, etc.).

Each item in the output list corresponds to an item in `Positions`. If a position appears twice in `Positions`, the item will appear twice in the output list. The order of items in the output list matches the order of `Positions` (even if the positions are not in ascending order).

If one of `Positions` is less than 1, the first item from the input list is picked. If one of `Positions` is greater than the list size, the last item is picked.

If the input list doesn't contain any items, the output list will contain a zero or empty value (for example, an empty image or blank text) for each of `Positions`.
