Creates an equirectangular image where each pixel uses periodic noise to blend between 2 colors, suitable for shading a sphere.

   - `Color A` and `B` — The colors to randomly blend.
   - `Center` — Moves the noise in 3D.
   - `Scale` — The size of the noise pattern.  At smaller values, the noise ripples are more closely packed together.
   - `Type` — Whether to render gradient (soft, Perlin-style) noise, value noise, cellular (ridged, Worley/Voronoi-style) noise, or a grid of dots.
   - `Grid` — Whether the noise grid is rectangular or triangular.
   - `Range` — Increases contrast by focusing on part of the output value range.  Values are clamped to the specified range, then normalized.
   - `Levels` (Octaves) — How many layers of noise to blend together.
   - `Roughness` (Persistence) — How much each level contributes to the composite image.  At 1, all noise levels are equal.  At 0.5, the second level is half the intensity of the first level, the third level is one quarter the intensity of the first, and so on.
   - `Spacing` (Lacunarity) — How much smaller each level is.  At 2, the second level is half the size of the first, the third level is one quarter the size of the first, and so on.
   - `Width` — The image's width, in pixels.  Since the image is equirectangular, its height is always half its width.
