Outputs information about a window.

   - `Width` — The width of the window's content area, in Vuo Coordinates.  Since the scene is scaled to match the width of the window, this is always 2.
   - `Height` — The height of the window's content area, in Vuo Coordinates.
   - `Pixels Wide`, `Pixels High` — The size of the window's content area, in pixels.  On Retina displays, these outputs represent the physical number of pixels (as opposed to logical 2-pixel-wide points).
   - `Aspect Ratio` — The ratio of the width to the height of the window's content area.  For example, a window that is twice as wide as it is high has an aspect ratio of 2.
   - `Top`, `Right`, `Bottom`, `Left` — The location of each side of the window's content area, in Vuo Coordinates.  Since the scene is scaled to match the width of the window, the left side is always -1, and the right side is always 1.
   - `Is Fullscreen` — True if the window is currently fullscreen.
