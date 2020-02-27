Loads or downloads a file containing XML-formatted text, and turns the text into a tree.

   - `Include Whitespace` — Indentation and line breaks between elements are treated as content if this is true and ignored if this is false.

See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.

If there's an error in the XML, this node outputs an empty tree. Check Console.app for details about the error.

The XML root element's name, attributes, and content become the tree's name, attributes, and content. The root element's child elements become the tree's children.

This node is a shortcut for [Fetch Data](vuo-node://vuo.data.fetch) -> [Convert Data to Text](vuo-node://vuo.type.data.text) -> [Make Tree from XML](vuo-node://vuo.tree.make.xml).
