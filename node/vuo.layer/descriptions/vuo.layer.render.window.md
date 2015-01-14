Displays a window containing a composite image. 

When the composition starts or this node is added to a running composition, it pops up a window that contains a graphics area.

The window provides a menu option to toggle between windowed and full-screen mode.

Mouse positions are in Vuo coordinates. 

   - `layers` — The layers that make up the composite image. The layers are placed on top of each other in the order listed, with the first layer on the bottom and the last layer on the top. 
   - `requestedFrame` — When the display is ready for the next frame, fires an event with information about the frame (including the time at which the frame will be rendered, and the total number of frames rendered so far).
   - `movedMouseTo` — When the mouse is moved while this is the active (frontmost) window, fires an event with the current position of the pointer.
   - `scrolledMouse` — When the mouse is scrolled, fires an event with the distance scrolled. This is the change in position since the previous `scrolled` event. The vertical distance (y-coordinate) respects the operating system's "natural scrolling" setting.
   - `usedMouseButton` — When a mouse button is pressed or released, fires an event with information about how the mouse button was used.
