Creates an orthographic camera that can be added to a 3D scene. 

The camera uses an [orthographic projection](http://en.wikipedia.org/wiki/Graphical_projection). Unlike a human eye or a typical camera lens, the orthographic projection makes objects appear the same size regardless of their distance from the camera. It doesn't have a vanishing point. 

   - `name` — A name to identify the camera, allowing other nodes to select it from a list of cameras. 
   - `position` — The position of the camera, in Vuo coordinates. 
   - `rotation` — The rotation of the camera, in degrees. If this is (0,0,0), the camera points toward negative infinity on the Z axis. 
   - `width` — The width of the area visible to the camera, in Vuo coordinates. The height is determined by the width and the aspect ratio when the scene is rendered. 
   - `distanceMin`, `distanceMax` — The minimum and maximum distance from the camera at which an object is visible (clipping planes), in Vuo coordinates. If `distanceMin` is negative, then objects behind the camera are visible. 
