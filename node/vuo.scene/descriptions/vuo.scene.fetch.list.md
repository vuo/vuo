Loads or downloads 3D scenes (3D models) from a set of URLs.

This node interprets the scene's vertices as Vuo coordinates. Optionally, this node can center and fit the scene to make sure it shows up within the area rendered by the `Render Scene to Window` or `Render Scene to Image` node (with the default camera). The centering and fitting are based on the bounding box around the scene — the smallest box that can enclose it. The bounding box is always aligned with the X-axis, Y-axis, and Z-axis (not rotated). If the scene is both centered and fit, then the centering is done first.

   - `URLs` — The files or links to the 3D models.
   - `Center` — If true, the scene is translated so that its bounding box is centered at the point (0,0,0). If false, the scene keeps its original center.
   - `Fit` — If true, the scene is scaled so that the width, height, and depth of its bounding box are at most 1. If false, the scene keeps its original size.
   - `Has Left Handed Coordinates` — If true, the scene is converted from a left-handed coordinate system to Vuo's right-handed coordinate system. If false, the scene is assumed to already use a right-handed coordinate system. (Most 3D modeling applications — including AutoCAD, Maya, 3ds Max, and Blender — use a right-handed coordinate system.)
   - `Scenes` — The scenes that are successfully loaded. If one of the URLs can't be loaded, then there's no item for it in this list.

To download a 3D scene from the internet, copy the scene's URL from your browser. Example: 

   - `http://example.com/bunny.3ds`

To load a 3D scene from a file on the computer running the composition, create a URL from the file's path. Examples: 

   - `file:///Users/me/Desktop/bunny.dae`
   - `/Users/me/Desktop/bunny.dae`
   - `bunny.dae` (for a file called "bunny.dae" in the same folder as the composition)
   - `scenes/bunny.dae` (for a file called "bunny.dae" in a folder called "scenes" in the same folder as the composition)

If the file's location doesn't start with `file://` or `/`, then it's treated as a relative path. This is the option you'd typically want when sharing the composition with others. If running the composition on a different computer than it was created on, the file needs to exist in the same location relative to the composition (`.vuo`) file. If running an application exported from the composition, the file needs to exist in the `Contents/Resources` folder inside the application package; see the [Vuo Manual](http://vuo.org/manual.pdf) for details. 

If the file's location starts with `file://` or `/`, then it's treated as an absolute path. If running the composition on a different computer than it was created on, the file needs to exist in the same location on that computer. To get the file's path in the correct format, open the TextEdit application, go to Format > Make Plain Text, drag the file onto the TextEdit window, and copy the path that appears in the TextEdit window. 

Example 3D models are available from [The Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/). 
