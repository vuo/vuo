Converts a 3D point to a transform that rotates an object.  The transform does not apply translation or scale.

When used with a `Combine Transforms` node, it is possible to easily construct a composite transform with a series of individual transformations.  This is useful in scenarios where it is necessary to apply parts of a transform in a specific order.  As an example, starting with an object at the origin and translating it -1 units on the Z axis, rotating 90° on the Y axis, then translating -1 units on the Z axis again will ultimately place the object at coordinates `(-1, 0, -1)`.
