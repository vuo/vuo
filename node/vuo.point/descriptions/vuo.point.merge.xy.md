Combines 2 lists of `X` and `Y` values into a single list of 2D points.

If either of these lists contains only 1 item, then that item will be used in every point.

If either of these lists contains at least 2 items, but fewer items than the longer list, then points created past the end of the shorter list will extrapolate from the last 2 items in that shorter list. For example, if `X` has 8 items and `Y` has only 2 items, 10 and 20, then the 1st point will have `Y` value 10, the 2nd point will have `Y` value 20, the 3rd point will have `Y` value 30, and so on, up to the 8th point with `Y` value 80.
