Outputs the content of a tree.

The content of a tree corresponds to the text enclosed by an element in XML or the value in an object's name-value pair in JSON. For example, the XML `<common-name>chimpanzee</common-name>` and the JSON `{"common-name" : "chimpanzee"}` both have content `chimpanzee`.

If `Include Descendants` is false, this node only outputs the content belonging to the tree itself. If `Include Descendants` is true, it also outputs the content belonging to the tree's descendants.
