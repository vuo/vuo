Fires an event when the mouse is moved. 

   - `Window` — If a window is provided, then mouse positions are in Vuo Coordinates relative to the window. Otherwise, mouse positions are in screen coordinates.  Additionally, if a window is provided and the composition app is focused, then mouse moves are fired only if the specified window is active (frontmost).
   - `Modifier Key` — The keyboard button (if any) that must be held for mouse moves to be tracked. 
   - `App Focus` — Whether to fire events only when the composition app is in the foreground, or to fire events regardless of whether the composition app is focused.
   - `Moved To` — When the mouse is moved, fires an event with the position that it was moved to. 

Mouse moves are tracked everywhere on the screen, including outside of the composition windows.

If `Window` is provided, and if `Modifier Key` is Any or None, this node fires the current mouse position when the composition starts and when changes are made while live coding.
