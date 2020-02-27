Creates a tree from components.

   - `Name` corresponds to the element name in XML or the name in an object's name-value pair in JSON.
   - `Attributes` correspond to the attributes of an element in XML.
   - `Content` corresponds to the text enclosed by an element in XML or the value in an object's name-value pair in JSON.
   - `Children` corresponds to the elements within an element in XML or the objects within an object in JSON.

For example, a tree with name `product`, content `Handcrafted pet rock`, an attribute named `sku` with value `rock`, and no children would be equivalent to XML `<product sku="rock">Handcrafted pet rock</product>`.

`Children` can be left empty, or it can consist of trees output by other instances of this node or by other nodes, such as [Make Tree from JSON](vuo-node://vuo.tree.make.json) or [Find Subtrees using XPath](vuo-node://vuo.tree.find.xpath).
