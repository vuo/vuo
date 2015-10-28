Centers and scales a 3D object so that it can easily be rendered.

This node can center and fit the scene to make sure it shows up within the area rendered by the `Render Scene to Window` or `Render Scene to Image` node (with the default camera). The centering and fitting are based on the bounding box around the scene — the smallest box that can enclose it. The bounding box is always aligned with the x-axis, y-axis, and z-axis (not rotated). If the scene is both centered and fit, then the centering is done first.

   - `Center` — If true, the scene is translated so that its bounding box is centered at the point (0,0,0). If false, the scene keeps its original center.
   - `Fit` — If true, the scene is scaled so that the width, height, and depth of its bounding box are at most 1. If false, the scene keeps its original size.
