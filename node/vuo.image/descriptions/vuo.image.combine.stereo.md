Merges two images — the left and right eyes — into a single image.

- `Stereo Type` — Describes how the images will be combined.  See below for an explanation of each type.
- `Left Image` — The left eye view image.
- `Right Image` — The right eye view image.

Stereo types:

- `Left by Right Half` — The `Left` and `Right` images are combined in a row, and horizontally resized to half their original width.  The combined image is the same size as the `Left` image.
- `Left by Right Full` — The `Left` and `Right` images are aligned in a row, and the output image is resized horizontally to accommodate both images at full resolution.
- `Top by Bottom Half` — The `Left` and `Right` images are stacked vertically, with each image being resized to half its original height so that the final combined image is same height as the `Left` input.
- `Top by Bottom Full` — The `Left` and `Right` images are stacked vertically, resizing the image height to accommodate both images at full resolution.  The `Left` image is the top.
- `Anaglyph: Red / Cyan` — Images are filtered by color, then combined to a single image.  The `Left` image is filtered through the red channel, and `Right` is a green / blue mix.  Currently the most commonly used anaglyph scheme.
- `Anaglyph: Amber / Blue` — Images are filtered by color, then combined to a single image.  The `Left` image is filtered through a red / green / gray mix, and `Right` is pure blue.  Also known as ColorCode 3-D.
- `Anaglyph: Green / Magenta` — Images are filtered by color, then combined to a single image.  The `Left` image is filtered through a green color, and `Right` is a mix of red and blue.  Also known as Trioscopic.
- `Vertical Stripe` — Pixels alternate by column between left and right image.  The left row is from the `Left` image.
- `Horizontal Stripe` — Pixels alternate by row between left and right image.  The top row is from the `Left` image.
- `Checkerboard` — Pixels alternate row and column between left and right image.  The top left pixel is the `Left` image.
- `Blend` — Both images are blended together equally.  This is useful for visualizing differences between images.
- `Difference` — Output is the difference in colors between each image.  This is useful for minimizing vertical alignment issues.
- `Left Only` — The output will be the unaltered `Left` image.
- `Right Only` — The output will be the unaltered `Right` image.
