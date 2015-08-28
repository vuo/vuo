Moves objects so that they are evenly arranged in a 3D grid.

This node takes the first object and places it in the top, left, front of the grid, and it continues rightward, then downward, then back.

   - `objects` — The objects to arrange in a grid.
   - `scaleToFit` — If true, each object is scaled proportionately to fit within its grid cell. If false, objects' scales are left unaltered.
   - `center` — The center position of the grid, in scene coordinates.
   - `width`, `height`, and `depth` — The size of the grid along the `x`, `y`, and `z` axis, in scene coordinates.
   - `rows`, `columns`, and `slices` — The number of grid cells along the `x`, `y`, and `z` axis.
