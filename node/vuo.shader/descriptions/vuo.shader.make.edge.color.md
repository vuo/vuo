Creates a graphics shader that paints a 3D object opaque near the edges and transparent in the center (or vice versa).

This shader works best for objects with curved surfaces — it uses the surface's angle relative to the camera to determine whether it's an "edge", so it shades flat surfaces with a solid color.

   - `Color` — The color to paint the edges and interior. The color's opacity affects the edge opacity; interior opacity can be independently controlled.
   - `Width` — How far to shade from the edges. Values near 0 produce thinly shaded edges; values near 1 shade most of the object.
   - `Sharpness` — How sharp the edge is. A value of 0 means the transition is very gradual; a value of 1 means the transition is immediate, creating a cartoonish effect.
   - `Interior Opacity` — How opaque the non-edge parts of the object are. `Interior Opacity` works even when `Color`'s opacity is zero, creating a ghostly effect.

This shader ignores lighting.
