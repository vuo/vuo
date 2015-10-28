Creates a graphics shader that can paint an image on a 3D object.

   - `Opacity` — The opacity of the 3D object. Ranges from 0 (fully transparent) to 1 (fully opaque). 
   - `Highlight Color` — The color of shiny (specular) reflections.
   - `Shininess` — How dull (0) or polished (1) the surface appears.

The image is stretched across the vertices of the 3D object. Unless the vertices form a rectangle whose aspect ratio matches the image's, the image will be deformed.

This shader causes the object to be lit by the lights in the scene.  (Parts of the object that face the light will be brighter than parts that face away from the light.)
