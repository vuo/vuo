Changes the dimensions of an image if this would make the image smaller. 

If `Width` is larger than the input image's width, or if `Height` is larger than the input image's height, then this node outputs an image with the given `Width` and `Height`. Otherwise, it outputs the original image. 

   - `Sizing Mode` — The way to scale the image if the given `Width` and `Height` have a different aspect ratio than the original image. 
      - `Stretch` — Scales and distorts the image to match the new dimensions and aspect ratio. 
      - `Fit` — Scales the image without distortion, making one dimension (width or height) match the given dimensions and the other fit inside. The remaining area (top/bottom or left/right edges) is transparent.
      - `Fill` — Scales the image without distortion, making one dimension (width or height) match the given dimensions and the other extend beyond. The parts of the image that don't fit (top/bottom or left/right edges) are cropped. 
      - `Proportional` — Like `Fit`, but with the transparent areas cropped out.
   - `Width` — The maximum width of the output image, in pixels. 
   - `Height` — The maximum height of the output image, in pixels. 
