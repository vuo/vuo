Creates a transform that can change a layer's position, rotation, or size.

   - `Translation` — The amount to shift the object's position, in Vuo Coordinates. If this is (0,0), the position will be unchanged.
   - `Rotation` — The amount to rotate the object counterclockwise, in degrees. If this is 0, the object's rotation will be unchanged.
   - `Scale` — The scale factor. If this is (1,1), the object's size will be unchanged.

When the transform is applied to a layer, the layer is first rotated, then scaled, then translated.
