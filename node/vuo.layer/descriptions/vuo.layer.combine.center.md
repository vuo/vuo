Groups multiple layers together and applies a transformation to the group. 

This node is useful for making a group of layers all move, rotate, or resize together. 

The position and rotation of the layer group are based on the bounding rectangle around the layers — the smallest rectangle that can enclose them. The bounding rectangle is always aligned with the X-axis and Y-axis (not rotated). For any Real Size layers in the group, the bounding rectangle will contain their center point but not necessarily the entire layer; only the center point is used when calculating the bounding rectangle. For other layers, the bounding rectangle will contain the entire layer.

   - `Anchor` — The point within the layer group's bounding rectangle that should be fixed at `Position`.
   - `Position` — The point within the composite image where the layer group should be placed, in Vuo Coordinates.
   - `Rotation` — The rotation of the combined layer about the center of its bounding rectangle, in degrees. For any Real Size layers in the group, rotating the group will affect the layer's position but not its rotation.
   - `Scale` — The scale factor of the combined layer compared to the input layers. At 1, the size of the layers is unchanged. For any Real Size layers in the group, scaling the group will affect the layer's position but not its size.
   - `Layers` — The layers to put into a group. When the layers are rendered, they're placed on top of each other in the order listed, with the first layer on the bottom and the last layer on the top.
