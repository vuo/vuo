Displays a window containing a composite image. 

When the composition starts or this node is added to a running composition, it pops up a window that contains a graphics area.

The window provides a menu option to toggle between windowed and full-screen mode.

   - `layers` — The layers that make up the composite image. The layers are placed on top of each other in the order listed, with the first layer on the bottom and the last layer on the top. 
   - `showedWindow` — When the window is displayed, fires an event with a reference to the window. It can be sent to user interaction nodes (such as `Receive Mouse Clicks`) to limit their scope to this window. 
   - `requestedFrame` — When the display is ready for the next frame, fires an event with the time at which the frame will be rendered.
   - `renderedLayers` — The layers, transformed to their positions in the composite image. 
