Creates a spotlight that can be added to a 3D scene.

A spotlight has a specific location (unlike ambient light), and only shines in one direction (unlike a point light).

For objects having shaders that respond to lighting, a spotlight causes parts of the object that face the light (and are within the spotlight's cone) to be brighter than parts that face away from the light (or are outside the spotlight's cone).

   - `Target` — The point toward which the light should shine, in Vuo Coordinates.
   - `Cone` — The angle of the spotlight's beam, in degrees.  Larger values mean a wider range of objects will be lit.
   - `Range` — The distance (in Vuo Coordinates) the light reaches.  For example, a value of 1 means that objects within a radius of 1 unit of the light will be lit.
   - `Sharpness` — How sharp the edge of the light is.  A value of 0 means the transition from light to dark is very gradual; a value of 1 means the transition is immediate.
