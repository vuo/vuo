Creates a transform that can change a 3D object's position, rotation, or size.

   - `Translation` — The amount to shift the object's position, in Vuo Coordinates. If this is (0,0,0), the position will be unchanged.
   - `Rotation` — The amount to rotate the object, in degrees (Euler angles). If this is (0,0,0), the object's rotation will be unchanged.
   - `Scale` — The scale factor. If this is (1,1,1), the object's size will be unchanged.

When the transform is applied to a 3D object, the object is first rotated, then scaled, then translated.

Rotation follows the [right-hand rule](https://en.wikipedia.org/wiki/Right-hand_rule#Rotations). If you point your right thumb in the positive direction of the axis while curling your fingers into a fist, your fingers curl in the direction of rotation. In other words, starting with a rotation of (0,0,0):

   - Increasing X rotation tilts the top of the object forward.
   - Increasing Y rotation turns the object to the viewer's right.
   - Increasing Z rotation spins the object counterclockwise from the viewer's perspective.

Rotation is applied in order of X, then Y, then Z.
