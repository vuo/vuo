Outputs information about a window.

   - `Window` — Which window to query.
   - `Unit` — Whether to provide the output coordinates in points, pixels, or Vuo Coordinates.
   - `Top Left`, `Bottom Right`
      - When Unit is set to Points or Pixels, these ports output the position, in [screen coordinates](vuo-nodeset://vuo.screen), of the top left or bottom right of the window's content area (excluding the window's title bar) relative to the top left of the main screen.
      - When Unit is set to Vuo Coordinates, these ports output the visible extent of the window's content area.  The left and right edges are always -1 and 1 respectively, since the scene is scaled to match the width of the window.
   - `Width`, `Height` — The size of the window's content area.  When Unit is set to Vuo Coordinates, Width is always 2 since the scene is scaled to match the width of the window.
   - `Aspect Ratio` — The ratio of the width to the height of the window's content area.  For example, a window that is twice as wide as it is high has an aspect ratio of 2.
