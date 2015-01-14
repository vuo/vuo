Loads or downloads a 3D scene (3D model) from a URL.

The vertices of the object should be in Vuo coordinates. If not, they can be scaled using the `Combine 3D Objects` node.

To download a 3D scene from the internet, copy the scene's URL from your browser. Example: 

   - `http://example.com/bunny.3ds`

Currently, URLs that begin with `https` are not supported.

To load a 3D scene from a file on the computer running the composition, create a URL from the scene file's path. Examples: 

   - `file:///Users/me/Desktop/bunny.dae`
   - `file:///Users/me/file\ with\ spaces.dxf` (for a file called `file with spaces.dxf`)

To get a file's path in the correct format, open the Terminal application, drag the file onto the Terminal window, and copy the path that appears in the Terminal window. Add `file://` at the beginning of the path.

Currently, only absolute file paths are supported. If you run a composition on a different computer than it was created on, then the 3D scene files need to be in the same location as on the original computer.

Example 3D models are available from [The Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/). 
