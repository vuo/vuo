Gets the current state of the mouse (or touchpad). Fires events when the mouse is clicked or moved.

The composition must have at least one window for this node to do anything.

Mouse positions and distances are in screen coordinates. The top left corner of the screen is (0,0). The bottom right corner of the screen matches the resolution of the display. For example, a 1440x900 display has bottom right coordinate (1400,900).

   - `position` — The current position of the pointer.
   - `isLeftPressed`, `isMiddlePressed`, `isRightPressed` — True if the left, middle, or right button is currently pressed.
   - `movedTo` — When the mouse is moved, fires an event with the current position of the pointer.
   - `movedBy` — When the mouse is moved, fires an event with the distance moved. This is the change in position since the previous `moved` event. If the pointer is already on the edge of the screen, the distance is non-zero even though the pointer stays in the same place on the screen.
   - `scrolled` — When the mouse is scrolled, fires an event with the distance scrolled. This is the change in position since the previous `scrolled` event. The vertical distance (y-coordinate) respects the operating system's "natural scrolling" setting.
   - `usedButton` — When a mouse button is pressed or released, fires an event with information about how the mouse button was used.
