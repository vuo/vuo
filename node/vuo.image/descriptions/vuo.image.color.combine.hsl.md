Combines hue, saturation, lightness, and opacity channels into a single image.

This node uses the input images as the hue, saturation, and lightness components of the output image.  The brighter (closer to white) an area of the input saturation or lightness images, the stronger that component will be in the output image.

The `Hue Image` input can either be color, in which case the image's color is used as the output image's hue, or grayscale, in which case the image's lightness is used as the output image's hue (black or white = red, ⅓ is green, and ⅔ is blue).

If an opacity image is provided, it acts as a mask on the combined image (light/opaque areas cause the combined image to be opaque; dark/transparent areas cause the combined image to be transparent).  If no opacity image is provided, the output image's opacity (alpha channel) is the average of the input images'.

The output image is the same size as `Hue Image`. The other input images are scaled to match.
