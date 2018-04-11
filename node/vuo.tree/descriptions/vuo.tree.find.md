Converts the data contained in a subtree to a chosen data type.

Searches the tree and each of its descendants for one whose name is `Name` (case-insensitive match). For the first matching subtree found, this node converts the data contained in the subtree to the chosen data type.

The default data type is Text. To choose a different data type, right-click on the output port and select Revert to Generic Data Type, then right-click on it again and select a data type.

If the chosen data type is Text, Integer, Real, or Boolean, this node interprets the subtree's content as that data type. For example:

<table>
   <tr><th>Data type</th><th>Tree</th><th>Output</th></tr>
   <tr><td>Integer</td><td>Tree with content <code>1024</code></td><td>1024</td></tr>
   <tr><td>Boolean</td><td>Tree with content <code>true</code></td><td><i>true</i></td></tr>
</table>

If the chosen data type is 2D Point, 3D Point, 4D Point, or Color, this node interprets the subtree's content or children as the components of that data type. For example:

<table>
   <tr><th>Data type</th><th>Tree</th><th>Output</th></tr>
   <tr><td>2D Point</td><td>Tree created from XML <code>&lt;location>&lt;x>1.2&lt;/x>&lt;y>3.4&lt;/y>&lt;/location></code></td><td>1.2, 3.4</td></tr>
   <tr><td>3D Point</td><td>Tree created from JSON <code>{"coordinates": {"x":1.2, "y":3.4, "z":5.6}}</code></td><td>1.2, 3.4, 5.6</td></tr>
   <tr><td>4D Point</td><td>Tree created from XML <code>&lt;point>&lt;x>1.2&lt;/x>&lt;y>3.4&lt;/y>&lt;z>5.6&lt;/z>&lt;w>7.8&lt;/w>&lt;/point></code></td><td>1.2, 3.4, 5.6, 7.8</td></tr>
   <tr><td>4D Point</td><td>Tree with content <code>1.2, 3.4, 5.6, 7.8</code></td><td>1.2, 3.4, 5.6, 7.8</td></tr>
   <tr><td>Color</td><td>Tree created from JSON <code>{"foreground": {"r":1, "g":0, "b":0, "a":1}}</code></td><td>opaque red</td></tr>
   <tr><td>Color</td><td>Tree with content <code>1.0, 0.0, 0.0, 1.0</code></td><td>opaque red</td></tr>
   <tr><td>Color</td><td>Tree with content <code>#00FF00</code></td><td>opaque green</td></tr>
</table>

This node is a shortcut for `Find Subtrees with Name -> Get First Item in List -> Convert Tree to Value`.
