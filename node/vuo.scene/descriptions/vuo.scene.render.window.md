Displays a window containing a 3D scene.

When the composition starts or this node is added to a running composition, it displays a window that contains a graphics area. The window provides a menu option to toggle between windowed and full-screen mode.

   - `Objects` — The 3D objects to place in the scene.
   - `Set Window Properties` — A list of properties that affect the appearance of the window or the way the user can interact with it. The properties are applied to the window in order from first to last in the list. Each property remains in effect until this port receives some other property that overrides it. 
   - `Camera Name` — The name of the camera through which the scene is viewed. The camera is selected from any in `Objects`. If no matching camera is found, then a camera from `Objects` is arbitrarily chosen. If there is no camera, the default camera is used. 
   - `Multisampling` — How smooth edges of objects are.  A value of 1 means each pixel is evaluated once — no smoothing.  A value of 2, 4, or 8 means each pixel that lies on the edge of an object is sampled multiple times and averaged together, to provide a smoother appearance.  Regardless of this setting, interior pixels are only evaluated once, so textures may still experience aliasing.  (File > Export > Movie's Antialiasing setting is an alternative that applies to the entire image, interiors included.)
   - `Showed Window` — Fires an event with a reference to the window when it is first shown, and whenever it is moved or resized. It can be sent to user interaction nodes (such as `Receive Mouse Clicks`) to limit their scope to this window. 
   - `Requested Frame` — When the display is ready for the next frame, fires an event with the time at which the frame will be rendered, measured in seconds since the composition started.
