Changes the dimensions of an image. 

   - `Sizing Mode` — The way to scale the image if the given `Width` and `Height` have a different aspect ratio than the original image. 
      - `Stretch` — Scales and distorts the image to match the new dimensions and aspect ratio. 
      - `Fit` — Scales the image without distortion, making one dimension (width or height) match the given dimensions and the other fit inside. The remaining area (top/bottom or left/right edges) is filled with transparent black. 
      - `Fill` — Scales the image without distortion, making one dimension (width or height) match the given dimensions and the other extend beyond. The parts of the image that don't fit (top/bottom or left/right edges) are cropped. 
   - `Width` — The width of the output image, in pixels. 
   - `Height` — The height of the output image, in pixels. 
