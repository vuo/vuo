Creates an image where each pixel uses periodic gradient noise to blend between 2 colors.

   - `Color A` and `B` — The colors to randomly blend.
   - `Center` — Moves the noise in 2D.
   - `Time` — The time at which to evaluate the image.  To animate the noise, connect a continuously increasing number, such as the output of the `Render Image to Window` node’s `Requested Frame` port.
   - `Scale` — The size of the noise pattern.  At smaller values, the noise ripples are more closely packed together.
   - `Width` — The image's width, in pixels.
   - `Height` — The image's height, in pixels.
