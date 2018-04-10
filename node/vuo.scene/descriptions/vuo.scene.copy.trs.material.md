Creates a 3D object group consisting of transformed copies of an original, each with a reassigned material.

This node is useful for displaying multiple similar 3D objects within a scene.

   - `Object` — The original 3D object.
   - `Materials` — A list of shaders, colors, or images to assign to each copy of the object.
   - `Material Extrapolation` — How to assign a material to each copy of the object:
      - Wrap:  The first copied object gets the first material; the second copied object gets the second material, and so on.  Upon reaching the end of the Materials list, the list wraps around, so the next copied object is assigned the first Material.
      - Stretch:  The materials are divided among the complete set of copied objects.  So if the Materials list contains 2 materials, the first half of the copied objects get the first material, and the second half gets the second material.
   - `Translations` — The copy's position, in Vuo Coordinates.
   - `Rotations` —  The copy's rotation, in degrees (Euler angles).
   - `Scales` — The copy's scale.
   - `Copies` — A group of 3D objects, with as many items as the longest list among `Materials`, `Translations`, `Rotations`, and `Scales`.

The 1st item in `Copies` has a transform created from the 1st item in `Translations`, `Rotations`, and `Scales`; the 2nd item has a transform created from the 2nd item in `Translations`, `Rotations`, and `Scales`; and so on.

If any of these lists is empty, this node outputs an empty object.

If any of these lists contains only 1 item, then that item will be used in every copy.

If `Translations`, `Rotations`, or `Scales` contains at least 2 items, but fewer items than the longest list, then copies created past the end of the shorter list will extrapolate from the last 2 items in that shorter list. For example, if `Translations` has 8 items and `Rotations` has only 2 items, (10,0,0) and (20,0,0), then the 1st copy will have rotation (10,0,0), the 2nd copy will have rotation (20,0,0), the 3rd copy will have rotation (30,0,0), and so on, up to the 8th copy with rotation (80,0,0).
