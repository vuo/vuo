Loads or downloads a 3D scene (3D model) from a URL.

The vertices of the object should be in Vuo coordinates. If not, they can be scaled using the `Combine 3D Objects` node.

To download a 3D scene from the internet, copy the scene's URL from your browser. Example: 

   - `http://example.com/bunny.3ds`

Currently, URLs that begin with `https` are not supported.

To load a 3D scene from a file on the computer running the composition, create a URL from the file's path. Examples: 

   - `file:///Users/me/Desktop/bunny.dae`
   - `/Users/me/Desktop/bunny.dae`
   - `bunny.dae` (for a file called "bunny.dae" in the same folder as the composition)
   - `scenes/bunny.dae` (for a file called "bunny.dae" in a folder called "scenes" in the same folder as the composition)

If the file's location doesn't start with `file://` or `/`, then it's treated as a relative path. This is the option you'd typically want when sharing the composition with others. If running the composition on a different computer than it was created on, the file needs to exist in the same location relative to the composition (`.vuo`) file. If running an application exported from the composition, the file needs to exist in the `Contents/Resources` folder inside the application package; see the [Vuo Manual](http://vuo.org/manual.pdf) for details. 

If the file's location starts with `file://` or `/`, then it's treated as an absolute path. If running the composition on a different computer than it was created on, the file needs to exist in the same location on that computer. To get the file's path in the correct format, open the TextEdit application, go to Format > Make Plain Text, drag the file onto the TextEdit window, and copy the path that appears in the TextEdit window. 

Example 3D models are available from [The Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/). 
