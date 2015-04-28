Creates a list of layers consisting of transformed copies of an original.

This node is useful for displaying multiple similar layers within a composite image.

   - `layer` — The original layer.
   - `translations` — The copy's position, in Vuo coordinates.
   - `rotations` —  The copy's rotation, in degrees.
   - `scales` — The copy's scale.
   - `copies` — A list of layers, with as many items as the longest list among `translations`, `rotations`, and `scales`.
   
The 1st item in `copies` has a transform created from the 1st item in `translations`, `rotations`, and `scales`; the 2nd item has a transform created from the 2nd item in `translations`, `rotations`, and `scales`; and so on. 

If any of these lists contains only 1 item, then that item will be used in every copy. 

If any of these lists contains at least 2 items, but fewer items than the longest list, then copies created past the end of the shorter list will extrapolate from the last 2 items in that shorter list. For example, if `translations` has 8 items and `rotations` has only 2 items, 10 and 20, then the 1st copy will have rotation 10, the 2nd copy will have rotation 20, the 3rd copy will have rotation 30, and so on, up to the 8th copy with rotation 80.
