Retrieves the color of part of an image.

- `Image` – The image to sample pixel colors from.
- `Center` – The position, in Vuo Coordinates relative to the image, to retrieve pixel data from.
- `Width` – The size, in Vuo Coordinates, of the square of pixels from `Image` that's examined to produce the output `Color`. If Width is 0, this node gets the color of just a single pixel from `Image`.
- `Sample Type` — How to determine the value for each RGBA component of the output color: the average of all pixels in the square, or the darkest or lightest component, or the darkest or lightest color.

Out-of-bounds coordinates are clamped to the edge of the image.  For example, `(-3, .2)` becomes `(-1, .2)`.
