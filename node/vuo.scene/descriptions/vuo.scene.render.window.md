Displays a window containing a 3D scene.

When the composition starts or this node is added to a running composition, it displays a window that contains a graphics area. The window provides a menu option to toggle between windowed and full-screen mode.

   - `objects` — The 3D objects to place in the scene.
   - `windowProperties` — A list of properties that affect the appearance of the window or the way the user can interact with it. The properties are applied to the window in order from first to last in the list. Each property remains in effect until this port receives some other property that overrides it. 
   - `cameraName` — The name of the camera through which the scene is viewed. The camera is selected from any in `objects`. If no matching camera is found, then a camera from `objects` is arbitrarily chosen. If there is no camera, the default camera is used. 
   - `showedWindow` — When the window is displayed, fires an event with a reference to the window. It can be sent to user interaction nodes (such as `Receive Mouse Clicks`) to limit their scope to this window. 
   - `requestedFrame` — When the display is ready for the next frame, fires an event with the time at which the frame will be rendered, measured in seconds since the composition started.
