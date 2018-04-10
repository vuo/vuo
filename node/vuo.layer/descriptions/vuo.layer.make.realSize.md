Creates a layer that can be combined with other layers to create a composite image. 

Since this node creates a Real Size layer, the image used to create the layer will be rendered at its original size and rotation in the composite image. If you want to scale and rotate the layer, use the `Make Image Layer (Scaled)` node instead.

   - `Name` — A name to identify the layer, allowing other nodes to select it from a group of layers. 
   - `Image` — The image displayed in the layer. For the layer to be visible, you have to provide an image. 
   - `Center` — The center point of the image, in Vuo Coordinates.  The center point is snapped so that it aligns exactly with a pixel.
   - `Opacity` — The image's opacity, from 0 (fully transparent) to 1 (fully opaque). 
   - `Preserve Physical Size` — When true, the image is rendered at the same physical size on both Retina and non-Retina screens, and the image's Scale Factor specifies the image's natural size (see [vuo.layer](vuo-nodeset://vuo.layer)).  When false, each pixel in the image is rendered as 1 pixel on the screen.
