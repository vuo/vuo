Replaces an item in a list.

`Position` is the number of the item to replace (1 for the first item, 2 for the second item, etc.). If `Position` is less than 1, the first item is replaced.

When `Expand List if Needed` is false:

- If `Position` is greater than the number of items in the list, the last item is replaced.

- If the input list doesn’t contain any items, this node outputs an empty list.

When `Expand List if Needed` is true:

- If `Position` is greater than the number of items in the list, the list is expanded (with empty values) to accommodate the specified position.
