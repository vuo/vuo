Creates a point light that can be added to a 3D scene.

A point light has a specific location (unlike ambient light) and shines in all directions (unlike a spotlight). 

For objects having shaders that respond to lighting, causes parts of the object that face the light to be brighter than parts that face away from the light.

   - `range` — The distance (in Vuo Coordinates) the light reaches.  For example, a value of 1 means that objects within a radius of 1 unit of the light will be lit.
   - `sharpness` — How sharp the edge of the light is.  A value of 0 means the transition from light to dark is very gradual; a value of 1 means the transition is immediate.
