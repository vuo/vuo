Creates a layer that can be combined with other layers to create a composite image. 

The image is displayed at real size — each pixel in the image is drawn as a single pixel in the output scene.

Real Size Layers can be translated, but cannot be rotated or scaled.

   - `image` — The image displayed in the layer. For the layer to be visible, you have to provide an image. 
   - `center` — The center point of the image, in Vuo coordinates.  The center point is snapped so that it aligns exactly with a pixel.
   - `alpha` — The image's opacity, from 0 (fully transparent) to 1 (fully opaque). 
