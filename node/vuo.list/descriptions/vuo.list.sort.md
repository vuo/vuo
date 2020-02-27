Sorts a list by its items' values.

The specific sort order depends on the type of the input items. For example, text is sorted alphabetically; real numbers and integers are sorted numerically. Points are sorted primarily by X-value, secondarily by Y-value, and so on. (To sort points by magnitude or distance from another point, use the [Sort Points by Distance](vuo-node://vuo.point.sort.distance) family of nodes instead.)

If `Sort Order` is Ascending, items are sorted from least to greatest. If `Sort Order` is Descending, items are sorted from greatest to least.

A given type might not have an intuitive sort order, but the ordering provided by this node will be consistent.
