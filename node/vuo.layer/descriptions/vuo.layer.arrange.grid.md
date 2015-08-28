Moves layers so that they are evenly arranged in a grid.

This node takes the first layer and places it in the top, left of the grid, and it continues rightward, then downward.

   - `layers` — The layers to arrange in a grid.
   - `scaleToFit` — If true, each scalable layer is scaled proportionately to fit within its grid cell. If false, layers' scales are left unaltered.  "Real size" layers are never scaled.
   - `center` — The center position of the grid, in scene coordinates.
   - `width` and `height` — The size of the grid along the `x` and `y` axis, in scene coordinates. 
   - `columns` and `rows` — The number of grid cells along the `x` and `y` axis.
