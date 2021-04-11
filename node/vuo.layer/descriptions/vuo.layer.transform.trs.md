Applies a transformation to a layer.

   - `Layer` — The original layer.
   - `Translation` — The new position, in Vuo Coordinates.
   - `Rotation` —  The new rotation, in degrees.
   - `Scale` — The new scale.

Unlike the [Combine Layers](vuo-node://vuo.layer.combine) nodes, this node doesn't put the layer in a group.  As a result some nodes that modify the transform may not work correctly when this node is used later in the composition (notably, [Receive Mouse Drags on Layer](vuo-node://vuo.layer.drag2)).
