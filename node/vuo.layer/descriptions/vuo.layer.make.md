Creates a layer that can be combined with other layers to create a composite image. 

The image used to create this layer may be resized in the composite image. If resizing makes the image larger, it may become blurry. If you want the image to be rendered at its original size, use the `Make Layer` node instead.

   - `Name` — A name to identify the layer, allowing other nodes to select it from a group of layers. 
   - `Image` — The image displayed in the layer. For the layer to be visible, you have to provide an image. 
   - `Center` — The center point of the image, in Vuo Coordinates. 
   - `Rotation` — The image's rotation counterclockwise, in degrees. 
   - `Width` — The image's width, in Vuo Coordinates. Its height is determined by the image's aspect ratio. 
   - `Opacity` — The image's opacity, from 0 (fully transparent) to 1 (fully opaque). 
