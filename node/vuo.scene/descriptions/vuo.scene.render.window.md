Displays a window containing a 3D scene.

When the composition starts or this node is added to a running composition, it pops up a window that contains a graphics area.

The window provides a menu option to toggle between windowed and full-screen mode.

Mouse positions are in Vuo coordinates. 

   - `objects` — The 3D objects to place in the scene.
   - `cameraName` — The name of the camera through which the scene is viewed. The camera is selected from any in `objects`. If no matching camera is found, then a camera from `objects` is arbitrarily chosen. If there is no camera, the default camera is used. 
   - `requestedFrame` — When the display is ready for the next frame, fires an event with information about the frame (including the time at which the frame will be rendered, and the total number of frames rendered so far).
   - `movedMouseTo` — When the mouse is moved while this is the active (frontmost) window, fires an event with the current position of the pointer.
   - `scrolledMouse` — When the mouse is scrolled, fires an event with the distance scrolled. This is the change in position since the previous `scrolled` event. The vertical distance (y-coordinate) respects the operating system's "natural scrolling" setting.
   - `usedMouseButton` — When a mouse button is pressed or released, fires an event with information about how the mouse button was used.
