Outputs *true* if all images have the same dimensions, and if their contents are close to equal.

If there are no images, outputs *true*.

`Tolerance` is the maximum amount by which any two component values can differ to still be considered equal. For example, in RGBA images, all red channels must be within `Tolerance`, and likewise for all green, blue, and alpha channels.

When comparing a 1-component image to a 4-component image, the 1-component image is treated as grayscale and opaque — i.e., for the images to be considered equal, the 4-component image must be grayscale and opaque within the specified `Tolerance`.

When comparing a 3-component image (no alpha) to a 4-component image (with alpha), the 3-component image's alpha is assumed to be 100%, so for the images to be considered equal, the 4-component image's alpha needs to be within `Tolerance` of 100%.
