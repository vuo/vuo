Finds all children of a tree that match a given name.

Compares the name of this tree and each of its children to `Name` using `Comparison`. For example, if `Comparison` is "begins with", this node searches for trees whose name begins with `Name`.  See the [Text node set documentation](vuo-nodeset://vuo.text) for more information.

If `Include Descendants` is false, this node only searches the immediate children of the tree. If `Include Descendants` is true, it also searches the tree's grandchildren and further descendants.
