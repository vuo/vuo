Creates a scene object in the shape of a tube.

- `Transform` — A transform that changes the tube's translation, rotation, or scale. It should use Vuo Coordinates.
- `Outside Material` — A shader, image, or color to apply to the outside surface of the tube.  To render without stretching, images should have an aspect ratio of 3.14:1 (about 3 times wider than tall).
- `Inside Material` — A shader, image, or color to apply to the inside surface of the tube.  To render without stretching, images should have an aspect ratio of 3.14:1 (about 3 times wider than tall) when `Thickness` is 0.  As `Thickness` increases, the image is squeezed, so you may want to provide an image with a taller aspect ratio.
- `Top Material` — A shader, image, or color to apply to the top surface of this tube.  To render without stretching, images should be square.
- `Bottom Material` — A shader, image, or color to apply to the bottom surface of this tube.  To render without stretching, images should be square.
- `Rows`/`Columns` — The number of vertices along the tube's latitude/longitude. With more vertices, the tube can be deformed more flexibly by object filter nodes, but may take longer to render.
- `Thickness` — The thickness of the pipe walls.  A value of 0 means the walls have no depth; the tube is hollow, and only `Outside Material` is used (the other materials are ignored).  A value of 1 means the tube is fully solid; the tube has no hole in the middle (and `Inside Material` is ignored).

By default, the tube has a diameter and height of 1 and is centered at (0,0,0).
