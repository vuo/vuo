Gives information about a layer.

   - `Transform` — The translation, rotation, and scale of this layer relative to its parent.
   - `Child Layer` — A list of this layer's children.

This node outputs the immediate children of the layer. To get a child layer's list of children and other information, pick that child layer from the output of this node and send it to another `Get Layer Values` node.
