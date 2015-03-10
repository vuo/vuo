Changes the dimensions of an image if this would make the image smaller. 

If `width` is larger than the input image's width, or if `height` is larger than the input image's height, then this node outputs an image with the given `width` and `height`. Otherwise, it outputs the original image. 

   - `sizingMode` — The way to scale the image if the given `width` and `height` have a different aspect ratio than the original image. 
      - `Stretch` — Scales and distorts the image to match the new dimensions and aspect ratio. 
      - `Fit` — Scales the image without distortion, making one dimension (width or height) match the given dimensions and the other fit inside. The remaining area (top/bottom or left/right edges) is filled with transparent black. 
      - `Fill` — Scales the image without distortion, making one dimension (width or height) match the given dimensions and the other extend beyond. The parts of the image that don't fit (top/bottom or left/right edges) are cropped. 
   - `width` — The maximum width of the output image, in pixels. 
   - `height` — The maximum height of the output image, in pixels. 
