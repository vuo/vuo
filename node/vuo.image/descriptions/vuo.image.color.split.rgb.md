Splits the image into its red, green, blue, and opacity channels.

Each color in a computer image is represented as a combination of [red, green, and blue components](http://en.wikipedia.org/wiki/RGB_color_model). For example, a pale pink color is made up of equal measures of red and blue, plus a little green.

This node outputs a grayscale image for each of the red, green, and blue components of the input image.  The lighter shades of gray in each output image correspond to areas where that color component was strongest in the input image.

If `Preserve Opacity` is true, the input image's opacity (alpha channel) is copied to the `Red/Green/Blue Image` output images.  If `Preserve Opacity` is false, the `Red/Green/Blue Image` output images are fully opaque.

The `Opacity Image` output is always fully opaque; its grayscale shades represent the opacity of the input image (lighter is more opaque; darker is more transparent).
