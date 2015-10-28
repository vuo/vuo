Adds an item to a list.

`Position` is the location in the list where the item should be inserted.  The item can be inserted at the beginning of the list, at the end of the list, or between two items in the list.

For example, if the input list contains 2 items:

   - `Position` 1 means the item would be placed before the 1st item.
   - `Position` 2 means the item would be placed between the 1st and 2nd items.
   - `Position` 3 means the item would be placed after the 2nd item.

If `Position` is less than 1, then the item is inserted at the beginning of the list.  If `Position` is greater than the number of items in the list, then the item is inserted at the end of the list.
