Outputs the center and size of a 3D object.

This node finds the center and size of the object's bounding box — the smallest box that can enclose the object. The bounding box is always aligned with the X-axis, Y-axis, and Z-axis (not rotated).

- `Center` - The center of the bounding box.
- `Width` - The size of the bounding box along the X-axis.
- `Height` - The size of the bounding box along the Y-axis.
- `Depth` - The size of the bounding box along the Z-axis.

For some objects, this node may take a long time to calculate the bounds. If it's slowing down your composition, try sending events to this node less often. For example, if animating an object that slightly changes shape over time, use this node to calculate the object's bounding box once when the composition starts rather than each time the object changes shape.
