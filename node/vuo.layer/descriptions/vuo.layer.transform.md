Applies a transformation to a layer.

Unlike the `Combine Layers` nodes, this node doesn't put the layer in a group.  As a result some nodes that modify the transform may not work correctly when this node is used later in the composition (notably, `Receive Mouse Drags on Layer`).
