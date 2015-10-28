Rounds a number or point to the nearest position on an imaginary grid. 

   - `Value` — The number or point to round.
   - `Spacing` — The spacing between numbers or points on the grid.
   - `Center` — A corner point in the grid. 

This node is useful for rounding or quantizing values so that they appear to snap to a grid of evenly spaced values. For example: 

   - If `Spacing` is 0.1 and `Center` is 0, then the snapped value will be a multiple of 0.1, such as 0, 0.1, -0.1, 0.2, or 1.3. 
   - If `Spacing` is (10,10) and `Center` is (5,5), then the snapped value will be a point such as (5,5), (15,5), (-5,15), or (105,105). 
