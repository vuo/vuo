Creates a graphics shader that can paint surface details on a 3D object.

   - `alpha` — The opacity of the 3D object. Ranges from 0 (fully transparent) to 1 (fully opaque). 
   - `specularImage` — An image that specifies the color and shininess of reflections.  The red, green, and blue channels affect the color of the specular reflections, and the alpha channel affects the shininess (transparent is dull; opaque is polished).  This image should be the same size as the main `image`.
   - `normalImage` — An image that specifies the surface details.  The red, green, and blue channels define the angle (3D normal vector) relative to a plane tangent to the object's surface — dark colors indicate that part of the surface is facing leftward (dark red) or upward (dark green); light colors indicate that part of the surface is facing rightward (light red) or downward (light green); neutral blue indicates that part of the surface is flat.  (The alpha channel is ignored.)

Each image is stretched across the vertices of the 3D object. Unless the vertices form a rectangle whose aspect ratio matches the images', the images will be deformed.

This shader causes the object to be lit by the lights in the scene.  (Parts of the object that face the light will be brighter than parts that face away from the light.)
