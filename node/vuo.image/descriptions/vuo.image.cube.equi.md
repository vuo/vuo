Warps an image from cubemap format to equirectangular format.

   - `Cubemap` — The cubemap to warp.
   - `Tilt`, `Pan`, `Roll` — The amount, in degrees, to rotate the cubemap along the X, Y, and Z axis respectively.  The rotations are performed in that same order: tilt, then pan, and finally roll.
   - `Width` — The width of the output image, in pixels.  The output image has aspect ratio 2:1, so the height of the output image is half of the specified width.  When set to `Auto`, the output image's width is 4 times the size of the largest input image.
   - `Equirectangular Image` — The resulting equirectangular image.

Thanks to [alexmitchellmus](https://vuo.org/user/2682) for contributing this node, and thanks to [Madsy](https://www.shadertoy.com/user/Madsy) for [the original shader this node was based on](https://www.shadertoy.com/view/4tjGW1).
