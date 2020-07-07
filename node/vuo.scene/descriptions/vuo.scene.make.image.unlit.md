Turns an image into a 3D object that can be added to a 3D scene. 

The 3D object is like a piece of paper that displays the image on one side.

   - `Center` — The center point of the 3D object, in Vuo Coordinates.
   - `Rotation` — The rotation of the 3D object, in degrees (Euler angles). At (0,0,0), the image is facing front.
   - `Size` — The 3D object's width or height, in Vuo Coordinates.  (See the `Fixed` port, below.)
   - `Fixed` — Whether the `Size` specifies the layer's width or height.  The other dimension is determined by the image's aspect ratio.
   - `Opacity` — The opacity of the 3D object. Ranges from 0 (fully transparent) to 1 (fully opaque).

This object ignores lighting.
