Creates a list of 3D transforms from lists of translation, rotation, and scale amounts.

The 1st item in `Transforms` has a transform created from the 1st item in `Translations`, `Rotations`, and `Scales`; the 2nd item has a transform created from the 2nd item in `Translations`, `Rotations`, and `Scales`; and so on.

If any of these lists is empty, this node outputs an empty list.

If any of these lists contains only 1 item, then that item will be used in every copy.

If any of these lists contains at least 2 items, but fewer items than the longest list, then copies created past the end of the shorter list will extrapolate from the last 2 items in that shorter list. For example, if `Translations` has 8 items and `Rotations` has only 2 items, (10,0,0) and (20,0,0), then the 1st copy will have rotation (10,0,0), the 2nd copy will have rotation (20,0,0), the 3rd copy will have rotation (30,0,0), and so on, up to the 8th copy with rotation (80,0,0).
