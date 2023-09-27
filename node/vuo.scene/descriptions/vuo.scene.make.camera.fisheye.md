Creates a fisheye camera that can be added to a 3D scene. 

This camera uses a [fisheye projection](https://en.wikipedia.org/wiki/Fisheye_lens).  It's capable of a very wide field of view — up to 180° (which produces a hemispherical image).

   - `Name` — A name to identify the camera, allowing other nodes to select it from a list of cameras. 
   - `Position` — The position of the camera, in Vuo Coordinates. 
   - `Rotation` — The rotation of the camera, in degrees. If this is (0,0,0), the camera points toward negative infinity on the Z axis. 
   - `Field of View` — The camera's [angle of view](https://en.wikipedia.org/wiki/Angle_of_view), in degrees.  When the rendering viewport is square and the Field of View is 180°, this will render a circular image circumscribing the viewport.  As you decrease the Field of View, the image starts to cover more of the rendering viewport.
   - `Distance Min`, `Distance Max` — The minimum and maximum distance from the camera at which an object is visible (clipping planes), in Vuo Coordinates.  These clipping planes are parallel to the camera's ground plane.

The fisheye projection causes straight lines in the scene to be rendered as curves.  To produce good-looking results, you should ensure that objects in your scene are sufficiently **tessellated**, such that the vertices are close together.  For example:

   1. When using nodes that generate objects (such as [Make 3D Square](vuo-node://vuo.scene.make.square)), try increasing the number of Rows, Columns, Slices, Points, or Subdivisions.
   2. When using [Fetch Scene](vuo-node://vuo.scene.fetch), try either using 3rd-party software to tessellate the mesh, or performing basic tessellation in Vuo using the [Spike 3D Object](vuo-node://vuo.scene.spike) node with `Distance` 0.
