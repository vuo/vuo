Creates a layer that can be combined with other layers to create a composite image.

The image used to create this layer may be resized in the composite image. If resizing makes the image larger, it may become blurry. If you want the image to be rendered at its original size, use the [Make Image Layer (Real Size)](vuo-node://vuo.layer.make.realSize2) node instead.

   - `Image` — The image displayed in the layer. For the layer to be visible, you have to provide an image.
   - `Anchor` — The point within the layer that should be fixed at `Position`.
   - `Position` — The point within the composite image where the layer should be placed, in Vuo Coordinates.
   - `Rotation` — The image's rotation counterclockwise, in degrees.
   - `Size` — The layer's width or height, in Vuo Coordinates.  (See the `Fixed` port, below.)
   - `Fixed` — Whether the `Size` specifies the layer's width or height.  The other dimension is determined by the image's aspect ratio.
   - `Opacity` — The image's opacity, from 0 (fully transparent) to 1 (fully opaque).
