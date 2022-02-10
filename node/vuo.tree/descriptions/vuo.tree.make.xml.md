Turns XML-formatted text into a tree.

   - `Include Whitespace` — Indentation and line breaks between elements are treated as content if this is true and ignored if this is false.

If there's an error in the XML, this node outputs an empty tree. Check Vuo's Console window (Tools > Show Console) for details about the error.

The XML root element's name, attributes, and content become the tree's name, attributes, and content. The root element's child elements become the tree's children.
