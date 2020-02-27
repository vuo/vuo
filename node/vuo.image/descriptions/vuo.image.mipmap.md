Makes the image smoother when its size is reduced.

By default, when an image is scaled down (both when using the `Resize Image` nodes and when changing the size of a Layer or 3D Object), each pixel in the output image only takes into account at most 4 pixels from the input image.  When significantly reducing the size of an image (more than a factor of 2), this can lead to flickering and moiré patterns because many pixels are ignored.

This node repeatedly scales down the image by a factor of 2, and instructs the GPU to make use of this series of images (collectively called a "mipmap") when it's rendered at smaller sizes.  This ensures that every pixel in the input image is considered, even when significantly reducing the size.

Since this precalculation takes some time and isn't always needed, and since the resulting mipmap requires about 50% more memory than the original image, Vuo does not enable it by default.
