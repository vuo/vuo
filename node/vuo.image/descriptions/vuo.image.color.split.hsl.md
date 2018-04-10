Splits the image into hue, saturation, lightness, and opacity channels.

This node outputs a grayscale image for each of the hue, saturation, and lightness components of the input image.  The lighter shades of gray in the saturation and lightness output images correspond to areas where saturation and lightness were higher in the input image.

If `Color Hue Image` is false, the `Hue Image` output is greyscale, where the lightness represents the hue (black or white = red, ⅓ is green, and ⅔ is blue).  If `Color Hue Image` is true, the `Hue Image` output shows the hue in color, with constant saturation and lightness.

If `Preserve Opacity` is true, the input image's opacity (alpha channel) is copied to the `Hue/Saturation/Lightness Image` output images.  If `Preserve Opacity` is false, the `Hue/Saturation/Lightness Image` output images are fully opaque.

The `Opacity Image` output is always fully opaque; its greyscale shades represent the opacity of the input image (lighter is more opaque; darker is more transparent).
