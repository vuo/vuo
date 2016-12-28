Loads or downloads 3D scenes (3D models) from a set of URLs.

This node interprets the scene's vertices as Vuo Coordinates. Optionally, this node can center and fit the scene to make sure it shows up within the area rendered by the `Render Scene to Window` or `Render Scene to Image` node (with the default camera). The centering and fitting are based on the bounding box around the scene — the smallest box that can enclose it. The bounding box is always aligned with the X-axis, Y-axis, and Z-axis (not rotated). If the scene is both centered and fit, then the centering is done first.

   - `URLs` — The files or links to the 3D models.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
   - `Center` — If true, the scene is translated so that its bounding box is centered at the point (0,0,0). If false, the scene keeps its original center.
   - `Fit` — If true, the scene is scaled so that the width, height, and depth of its bounding box are at most 1. If false, the scene keeps its original size.
   - `Has Left Handed Coordinates` — If true, the scene is converted from a left-handed coordinate system to Vuo's right-handed coordinate system. If false, the scene is assumed to already use a right-handed coordinate system. (Most 3D modeling applications — including AutoCAD, Maya, 3ds Max, and Blender — use a right-handed coordinate system.)
   - `Scenes` — The scenes that are successfully loaded. If one of the URLs can't be loaded, then there's no item for it in this list.

Example 3D models are available from [The Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/). 
