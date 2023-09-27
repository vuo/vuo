Shrinks the image to its center lines.

This node applies a morphological skeletonization filter to the input image, which attempts to identify connected regions of the image by repeatedly eroding the image and comparing it to the previously eroded image.  It can be used to reduce the amount of information in an image (i.e., distill or pare it down to its essential structure).

   - `Image` — The image to filter.
   - `Radius` — How far to erode the image on each iteration.  A radius of 1 produces a canonical skeleton.
   - `Iterations` — How many times to repeat the erode-and-compare process.  If Auto, the process repeats until there is nothing left to erode, producing a canonical skeleton.

![](helloworld.png)

![](skeletonize.png)
