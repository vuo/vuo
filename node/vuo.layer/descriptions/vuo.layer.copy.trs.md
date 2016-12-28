Creates multiple layers consisting of transformed copies of an original.

This node is useful for displaying multiple similar layers within a composite image.

   - `Layer` — The original layer.
   - `Translations` — The copy's position, in Vuo Coordinates.
   - `Rotations` —  The copy's rotation, in degrees.
   - `Scales` — The copy's scale.
   - `Copies` — A layer group containing the layer copies, with as many items as the longest list among `Translations`, `Rotations`, and `Scales`.
   
The 1st item in `Copies` has a transform created from the 1st item in `Translations`, `Rotations`, and `Scales`; the 2nd item has a transform created from the 2nd item in `Translations`, `Rotations`, and `Scales`; and so on. 

If any of these lists is empty, this node outputs an empty layer.

If any of these lists contains only 1 item, then that item will be used in every copy. 

If any of these lists contains at least 2 items, but fewer items than the longest list, then copies created past the end of the shorter list will extrapolate from the last 2 items in that shorter list. For example, if `Translations` has 8 items and `Rotations` has only 2 items, 10 and 20, then the 1st copy will have rotation 10, the 2nd copy will have rotation 20, the 3rd copy will have rotation 30, and so on, up to the 8th copy with rotation 80.
