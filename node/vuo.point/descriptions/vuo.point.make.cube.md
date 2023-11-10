Calculates a list of points along the edges of a cube.

   - `Center` — The center position of the cube, in Vuo Coordinates.
   - `Size` — The length of each of the cube's edges, in Vuo Coordinates.  To individually specify the width, height, and depth, change this port to type _3D Point_.
   - `Edge Offset` — Pushes each of the cube's faces inward or outward by a distance in Vuo Coordinates.
   - `X Curve`, `X Easing`, `Y Curve`, `Y Easing`, `Z Curve`, `Z Easing` — The shape of the [easing curve](vuo-nodeset://vuo.motion) that defines how the points are spaced.
   - `X Point Count`, `Y Point Count`, `Z Point Count` — How many points to create along each axis.

This node outputs points for cube faces in the order front, left, right, back, top, bottom.  It individually creates points for each edge of each cube face; each shared edge is included twice (for example, the top-front edge is included for both the front face and the top face).

Thanks to [Martinus Magneson](https://community.vuo.org/u/MartinusMagneson) for developing the node this is based on.
