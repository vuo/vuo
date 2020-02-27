Outputs the current mouse position and buttons pressed. 

   - `Window` — If a window is provided, then mouse presses are only tracked when the mouse was pressed on that window, mouse positions are only tracked when that window is the active (frontmost) window, and mouse positions are in Vuo Coordinates relative to the window. Otherwise, mouse presses and releases are tracked when the mouse was pressed on any window of the composition, mouse positions are tracked whenever the composition is the active (frontmost) application, and mouse positions are in screen coordinates. In either case, mouse moves are tracked everywhere on the screen, including outside of the composition windows.
   - `Button` — The mouse button(s) to track. 
   - `Modifier Key` — The keyboard button (if any) that must be held for mouse buttons to be tracked. 
   - `Position` — The current position of the pointer. 
   - `Is Pressed` — True if the chosen button is currently pressed. 
