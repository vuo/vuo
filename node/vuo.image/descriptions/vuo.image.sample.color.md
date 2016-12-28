Retrieves the color of part of an image.

- `Image` – The image to sample pixel colors from.
- `Center` – The coordinate (in Vuo Coordinates) to retrieve pixel data from: -1 to 1 on the X axis.
- `Width` – The size, in Vuo Coordinates, of the square of pixels from `Image` that's averaged to produce the output `Color`. If Width is 0, this node gets the color of just a single pixel from `Image`.

Out of bounds coordinates are clamped to the edge of the image.  For example, `(-3, .2)` becomes `(-1, .2)`.
