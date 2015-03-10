Changes the dimensions of an image. 

   - `sizingMode` — The way to scale the image if the given `width` and `height` have a different aspect ratio than the original image. 
      - `Stretch` — Scales and distorts the image to match the new dimensions and aspect ratio. 
      - `Fit` — Scales the image without distortion, making one dimension (width or height) match the given dimensions and the other fit inside. The remaining area (top/bottom or left/right edges) is filled with transparent black. 
      - `Fill` — Scales the image without distortion, making one dimension (width or height) match the given dimensions and the other extend beyond. The parts of the image that don't fit (top/bottom or left/right edges) are cropped. 
   - `width` — The width of the output image, in pixels. 
   - `height` — The height of the output image, in pixels. 
