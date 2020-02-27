Emphasizes an image's edges.

This node creates an "unsharp mask" by blurring the image then subtracting it from the original image, leaving only the sharp pixels.  Then it adds that mask to the original image.

   - `Radius` — The distance, in points, to blur the image when creating the unsharp mask.
   - `Amount` — How strongly to apply the unsharp mask.  At 0, the input image is unchanged.  At 1, the unsharp mask is applied at its normal intensity.  At 5, the unsharp mask is applied at 5 times its normal intensity.
   - `Threshold` — The cutoff point when applying the unsharp mask, so that only the sharper edges are emphasized.  That is, the unsharp mask will be added to parts of the image where the brightness (tonal value) of the unsharp mask is greater than this threshold.  Increase this to avoid sharpening small details such as noise and image compression artifacts.

For more information, see [this article](https://en.wikipedia.org/wiki/Unsharp_masking).
