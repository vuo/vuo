Creates a layer that can be combined with other layers to create a composite image. 

The image used to create this layer will be rendered at its original size and rotation in the composite image. If you want to scale and rotate the layer, use the `Make Scaled Layer` node instead.

   - `Name` — A name to identify the layer, allowing other nodes to select it from a group of layers. 
   - `Image` — The image displayed in the layer. For the layer to be visible, you have to provide an image. 
   - `Center` — The center point of the image, in Vuo Coordinates.  The center point is snapped so that it aligns exactly with a pixel.
   - `Opacity` — The image's opacity, from 0 (fully transparent) to 1 (fully opaque). 
