Displays a window containing an image.

When the composition starts or this node is added to a running composition, it pops up a window that contains a graphics area.

When this node receives an event, it places the image in the graphics area. The window is resized to fit the image.

When the user resizes the window, the window's aspect ratio is kept the same as the image's.

The window provides a menu option to toggle between windowed and full-screen mode.

When the display is ready for the next frame, this node fires an event with information about the frame (including the time at which the frame will be rendered, and the total number of frames rendered so far).
