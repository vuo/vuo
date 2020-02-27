Warps an image from equirectangular (spherical) format to circular fisheye format.

   - `Equirectangular Image` — The image to warp.  Its aspect ratio should be 2:1.
   - `Field of View` — The output image's [angle of view](https://en.wikipedia.org/wiki/Angle_of_view), in degrees.
   - `Tilt`, `Pan`, `Roll` — The amount, in degrees, to rotate the image along the X, Y, and Z axis respectively.  The rotations are performed in that same order: tilt, then pan, and finally roll.
   - `Opaque Edges` — Whether to make the areas outside the circular fisheye image solid black (if true) or transparent (if false).
   - `Edge Width` — The size of the circle, in Vuo Coordinates.
   - `Edge Sharpness` — How sharp the edge of the circle is.  A value of 0 means the edge is very soft; a value of 1 means the edge is sharp.
   - `Width` — The size of the output image, in pixels.  The output image is a square — equal width and height.  When set to `Auto`, the smaller dimension (height) of the input image is used.
   - `Fisheye Image` — The resulting circular fisheye image.

[“Fisheye projections from spherical maps”](http://paulbourke.net/dome/2fish/) has more information on this image warping technique.

Thanks to [Paul Bourke](https://vuo.org/user/2025) for contributing this node.
