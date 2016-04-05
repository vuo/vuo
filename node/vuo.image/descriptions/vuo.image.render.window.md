Displays a window containing an image.

When the composition starts or this node is added to a running composition, it displays a window that contains a graphics area. The window provides a menu option to toggle between windowed and full-screen mode.

When this node receives an event, it places the image in the graphics area. The window is resized to fit the image, unless a `Change Window Size` or `Change Window Aspect Ratio` window property is in effect (in which case the `Reset Window Aspect Ratio` window property will return the window to its original aspect ratio and resume automatic resizing).

When the user resizes the window, the window's aspect ratio is kept the same as the image's.

   - `Set Window Properties` — A list of properties that affect the appearance of the window or the way the user can interact with it. The properties are applied to the window in order from first to last in the list. Each property remains in effect until this port receives some other property that overrides it. 
   - `Showed Window` — Fires an event with a reference to the window when it is first shown, and whenever it is moved or resized. It can be sent to user interaction nodes (such as `Receive Mouse Clicks`) to limit their scope to this window. 
   - `Requested Frame` — When the display is ready for the next frame, fires an event with the time at which the frame will be rendered, measured in seconds since the composition started.
