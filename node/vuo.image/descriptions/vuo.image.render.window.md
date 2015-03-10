Displays a window containing an image.

When the composition starts or this node is added to a running composition, it pops up a window that contains a graphics area.

When this node receives an event, it places the image in the graphics area. The window is resized to fit the image.

When the user resizes the window, the window's aspect ratio is kept the same as the image's.

The window provides a menu option to toggle between windowed and full-screen mode.

   - `showedWindow` — When the window is displayed, fires an event with a reference to the window. It can be sent to user interaction nodes (such as `Receive Mouse Clicks`) to limit their scope to this window. 
   - `requestedFrame` — When the display is ready for the next frame, fires an event with the time at which the frame will be rendered.
