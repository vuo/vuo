Steps forward or backward through a list and outputs the current list item. 

The first event to the `goForward` port outputs the first item in the list.  Each subsequent event outputs the next item.

An event to the `goBackward` port goes back to the previous item.

An event to the `goToFirst` port resets the node, so that the next event to `goForward` will output the first item. 
