Gives information about a 3D object.

   - `Name` — The name of this object (as set by a 3D modeling application).
   - `Transform` — The translation, rotation, and scale of this object relative to its parent.
   - `Child Objects` — A list of this object's children.

This node outputs the immediate children of the object. To get a child object's list of children and other information, pick that child object from the output of this node and send it to another `Get 3D Object Values` node.
