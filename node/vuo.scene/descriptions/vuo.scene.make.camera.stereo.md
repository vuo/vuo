Creates a stereoscopic perspective camera that can be added to a 3D scene. 

The camera uses an [asymmetric frustum](http://paulbourke.net/stereographics/stereorender/) [perspective projection](http://en.wikipedia.org/wiki/Graphical_projection). Like a human eye or a typical camera lens, the perspective projection makes near objects seem larger than distant objects. The camera produces two images: one for the left eye, and one for the right eye.

Use this in conjunction with the `Render Scene to Stereo Images` node.

   - `Name` — A name to identify the camera, allowing other nodes to select it from a list of cameras. 
   - `Position` — The position of the camera, in Vuo Coordinates. 
   - `Rotation` — The rotation of the camera, in degrees. If this is (0,0,0), the camera points toward negative infinity on the Z axis. 
   - `Field Of View` — The camera's [angle of view](http://en.wikipedia.org/wiki/Angle_of_view), in degrees. 
   - `Distance Min`, `Distance Max` — The minimum and maximum distance in front of the camera at which an object is visible (clipping planes), in Vuo Coordinates. These should be greater than 0. 
   - `Distance To Focal Plane` — The distance from the camera, in Vuo Coordinates, at which the left and right eye see the same image. Parts of objects that are closer or further than the confocal distance appear horizontally shifted between the left and right images.
   - `Distance Between Eyes` — The separation between the left and right cameras, in Vuo Coordinates.
