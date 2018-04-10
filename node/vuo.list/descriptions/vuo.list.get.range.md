Selects items that fall within specified ranges in a list.

`Ranges` contain the numbers of the items to pick (1 for the first item, 2 for the second item, etc.). All items from the minimum to the maximum of the range (including the minimum and maximum themselves) are selected.

The order of items in the output list matches the order of `Ranges` (even if the ranges are not in ascending order). If any ranges overlap, some items will appear multiple times in the output list.

If all or part of a range falls outside of the input list (less than 1 or greater than the list size), nothing will appear in the output list for the parts of the range outside of the list. Items will still appear for any parts of the range inside of the list.
