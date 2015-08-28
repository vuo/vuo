Outputs the size of the window. 

   - `width` — The width of the window's content area, in Vuo Coordinates.  Since the scene is scaled to match the width of the window, this is always 2.
   - `height` — The height of the window's content area, in Vuo Coordinates.
   - `pixelsWide`, `pixelsHigh` — The size of the window's content area, in pixels.  On Retina displays, these outputs represent the physical number of pixels (as opposed to logical 2-pixel-wide points).
   - `aspectRatio` — The ratio of the width to the height of the window's content area.  For example, a window that is twice as wide as it is high has an aspect ratio of 2.
   - `top`, `right`, `bottom`, `left` — The location of each side of the window's content area, in Vuo Coordinates.  Since the scene is scaled to match the width of the window, the left side is always -1, and the right side is always 1.
