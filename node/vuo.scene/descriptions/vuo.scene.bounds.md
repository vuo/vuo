Outputs the center and size of a 3D object.

This node finds the center and size of the object's bounding box — the smallest box that can enclose the object. The bounding box is always aligned with the x-axis, y-axis, and z-axis (not rotated).

- `center` - The center of the bounding box.
- `width` - The size of the bounding box along the x-axis.
- `height` - The size of the bounding box along the y-axis.
- `depth` - The size of the bounding box along the z-axis.

For some objects, this node may take a long time to calculate the bounds. If it's slowing down your composition, try sending events to this node less often. For example, if animating an object that slightly changes shape over time, use this node to calculate the object's bounding box once when the composition starts rather than each time the object changes shape.
