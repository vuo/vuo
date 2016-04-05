Periodically takes a screenshot of part of the screen.

You can use this node to capture the output of another app and feed it into your Vuo composition.  (If the app supports Syphon, you could alternatively use Vuo's `Receive Syphon Image` node.)

   - `Screen` — Which display to capture. If no screen is specified, a region is captured from the primary screen.  (In System Preferences > Displays > Arrangement, the primary screen is the one with the menu bar.)
   - `Top Left` — The coordinates, relative to `Screen`, of the top-left corner of the region to capture, in points (not pixels).
   - `Width` and `Height` — The size of the rectangle to capture, in points (not pixels).  If `Width` and `Height` are 0, the entire screen rightward/downward from `Top Left` is captured.

The capture rectangle is cropped to the visible area of `Screen`.  For example, if `Left` + `Width` goes beyond the right of of the screen, the output image will be smaller than `Width`.

Since the input location and size are in points, on Retina displays, the output image will have twice the number of pixels in each dimension.

To easily find screen coordinates, either use Command-Shift-4 or open the Preview app, go to File > Take Screen Shot > From Selection… In either case, the cursor will change to a crosshair and show the screen coordinate position in points. Use the escape key to exit.

