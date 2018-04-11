Creates an equirectangular image where each pixel uses periodic noise to blend between 2 colors, suitable for shading a sphere.

   - `Color A` and `B` — The colors to randomly blend.
   - `Center` — Moves the noise in 3D.
   - `Scale` — The size of the noise pattern.  At smaller values, the noise ripples are more closely packed together.
   - `Type` — Whether to render gradient (soft, Perlin-style) noise, value noise, cellular (ridged, Worley/Voronoi-style) noise, or a grid of dots.
   - `Grid` — Whether the noise grid is rectangular or triangular.
   - `Range` — Focus on part of the output value range.
   - `Range Mode` — How to determine the color of values outside `Range`:
      - None: Values outside `Range` extend beyond `Color A` and `B`.
      - Clamp Edge: Values are clamped to `Range`, then normalized to be between `Color A` and `B`.
      - Repeat: Values outside `Range` wrap around from `Color A` to `B` and vice-versa, producing sharp edges at `Range`'s bounds.
      - Mirror Repeat: Values outside `Range` bounce back toward the opposite end of the range, producing smooth edges at `Range`'s bounds.
   - `Levels` (Octaves) — How many layers of noise to blend together.
   - `Roughness` (Persistence) — How much each level contributes to the composite image.  At 1, all noise levels are equal.  At 0.5, the second level is half the intensity of the first level, the third level is one quarter the intensity of the first, and so on.
   - `Spacing` (Lacunarity) — How much smaller each level is.  At 2, the second level is half the size of the first, the third level is one quarter the size of the first, and so on.
   - `Width` — The image's width, in pixels.  Since the image is equirectangular, its height is always half its width.
