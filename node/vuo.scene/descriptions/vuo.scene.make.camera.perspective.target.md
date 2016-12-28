Creates a perspective camera that can be added to a 3D scene. 

The camera uses a [perspective projection](http://en.wikipedia.org/wiki/Graphical_projection). Like a human eye or a typical camera lens, the perspective projection makes near objects seem larger than distant objects. 

   - `Name` — A name to identify the camera, allowing other nodes to select it from a list of cameras. 
   - `Position` — The position of the camera, in Vuo Coordinates. 
   - `Target` — The point toward which the camera should face, in Vuo Coordinates.
   - `Up Direction` — The direction the top of the camera should face.
   - `Field Of View` — The camera's [angle of view](http://en.wikipedia.org/wiki/Angle_of_view), in degrees. 
   - `Distance Min`, `Distance Max` — The minimum and maximum distance in front of the camera at which an object is visible (clipping planes), in Vuo Coordinates. These should be greater than 0. 
