Creates a list of 3D objects consisting of transformed copies of an original.

This node is useful for displaying multiple similar 3D objects within a scene.

   - `object` — The original 3D object.
   - `translations` — The copy's position, in Vuo coordinates.
   - `rotations` —  The copy's rotation, in degrees (Euler angles).
   - `scales` — The copy's scale.
   - `copies` — A list of 3D objects, with as many items as the longest list among `translations`, `rotations`, and `scales`.
   
The 1st item in `copies` has a transform created from the 1st item in `translations`, `rotations`, and `scales`; the 2nd item has a transform created from the 2nd item in `translations`, `rotations`, and `scales`; and so on. 

If any of these lists contains only 1 item, then that item will be used in every copy. 

If any of these lists contains at least 2 items, but fewer items than the longest list, then copies created past the end of the shorter list will extrapolate from the last 2 items in that shorter list. For example, if `translations` has 8 items and `rotations` has only 2 items, (10,0,0) and (20,0,0), then the 1st copy will have rotation (10,0,0), the 2nd copy will have rotation (20,0,0), the 3rd copy will have rotation (30,0,0), and so on, up to the 8th copy with rotation (80,0,0).
