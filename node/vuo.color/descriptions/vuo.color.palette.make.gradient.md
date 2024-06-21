Creates a color palette that transitions between the given anchor colors.

   - `Anchor Colors` — The colors to transition between, in order from start to end. Each of these colors will appear in `Palette`.
   - `Tween Color Count` — How many additional colors to add to the palette between the anchor colors. The tween colors are linearly interpolated from one anchor color to the next.

If there are fewer than 2 anchor colors, no tween colors are added to the palette.
