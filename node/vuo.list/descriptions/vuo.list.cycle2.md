Steps forward or backward through a list and outputs the current list item.

The first event to the `Go Forward` port outputs the first item in the list.  Each subsequent event outputs the next item.

An event to the `Go Backward` port goes back to the previous item.

An event to the `Go To First` port resets the node, so that the next event to `Go Forward` will output the first item.

`Position` is the location of the current item in the list, with 1 being the first item.
