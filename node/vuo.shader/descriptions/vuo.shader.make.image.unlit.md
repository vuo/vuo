Creates a graphics shader that can paint an image on a 3D object.

   - `Opacity` — The opacity of the 3D object. Ranges from 0 (fully transparent) to 1 (fully opaque). 

The image is stretched across the vertices of the 3D object. Unless the vertices form a rectangle whose aspect ratio matches the image's, the image will be deformed.

This shader ignores lighting.
