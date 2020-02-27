Creates a composite image. 

   - `Layers` — The layers that make up the composite image. The layers are placed on top of each other in the order listed, with the first layer on the bottom and the last layer on the top. 
   - `Width`, `Height` — The width and height of the resulting image, in pixels.
   - `Color Depth` — The number of bits to use to represent each color channel in the output image; higher values are more precise but use more graphics storage space.  Images are typically 8 bits per channel ("8bpc"), though if you're working with image feedback, you may want to use higher precision.
   - `Multisampling` — How smooth edges of layers are.
      - A value of 1 means each pixel is evaluated once — no smoothing.
      - A value of 2, 4, or 8 means each pixel that lies on the edge of a layer is sampled multiple times and averaged together, to provide a smoother appearance.
   - `Image` — The composite image. If no layer is visible in part of the image, that part will be transparent.

### Antialiasing

If the graphics rendered by this node look jagged or blocky, you can…

   - Smooth the edges of layers by increasing `Multisampling`.
   - Smooth the interiors of image layers by using [Improve Downscaling Quality](vuo-node://vuo.image.mipmap).
   - Smooth an exported movie with the Antialiasing setting under File > Export > Movie…
