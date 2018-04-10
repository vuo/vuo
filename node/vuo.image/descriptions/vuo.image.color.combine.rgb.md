Combines red, green, blue, and opacity channels into a single image.

Each color in a computer image is represented as a combination of [red, green, and blue components](http://en.wikipedia.org/wiki/RGB_color_model). For example, a pale pink color is made up of equal measures of red and blue, plus a little green.

This node uses the input images as the red, green, and blue components of the output image.  The brighter (closer to white) an area of an input image, the stronger that color component will be in the output image.

If an opacity image is provided, it acts as a mask on the combined image (light/opaque areas cause the combined image to be opaque; dark/transparent areas cause the combined image to be transparent).  If no opacity image is provided, the output image's opacity (alpha channel) is the average of the input images'.

The output image is the same size as `Red Image`. The other input images are scaled to match.
