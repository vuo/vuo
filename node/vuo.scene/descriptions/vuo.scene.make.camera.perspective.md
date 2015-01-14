Creates a perspective camera that can be added to a 3D scene. 

The camera uses a [perspective projection](http://en.wikipedia.org/wiki/Graphical_projection). Like a human eye or a typical camera lens, the perspective projection makes near objects seem larger than distant objects. 

   - `name` — A name to identify the camera, allowing other nodes to select it from a list of cameras. 
   - `position` — The position of the camera, in Vuo coordinates. 
   - `rotation` — The rotation of the camera, in degrees. If this is (0,0,0), the camera points toward negative infinity on the Z axis. 
   - `fieldOfView` — The camera's [angle of view](http://en.wikipedia.org/wiki/Angle_of_view), in degrees. 
   - `distanceMin`, `distanceMax` — The minimum and maximum distance in front of the camera at which an object is visible (clipping planes), in Vuo coordinates. These should be greater than 0. 
