Creates a fisheye camera that can be added to a 3D scene. 

The camera uses a [fisheye projection](https://en.wikipedia.org/wiki/Fisheye_lens).  It's capable of a very wide field of view — up to 180° (which produces a hemispherical image).

Because of the wide field of view, straight lines in the scene are rendered as curves.  To produce good-looking results, you should ensure that objects in your scene are subdivided such that the vertices are close together.  (I.e., if you have a line made up of 2 points, there's no way to make the line curved.  But if you subdivide the line into many colinear points, the fisheye camera is able to reposition the points to form a curve.)

   - `Name` — A name to identify the camera, allowing other nodes to select it from a list of cameras. 
   - `Position` — The position of the camera, in Vuo Coordinates. 
   - `Target` — The point toward which the camera should face, in Vuo Coordinates.
   - `Up Direction` — The direction the top of the camera should face.
   - `Field of View` — The camera's [angle of view](http://en.wikipedia.org/wiki/Angle_of_view), in degrees.  When the rendering viewport is square and the Field of View is 180°, this will render a circular image circumscribing the viewport.  As you decrease the Field of View, the image starts to cover more of the rendering viewport.
