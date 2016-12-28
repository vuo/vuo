Fires an event when the mouse is moved. 

   - `Window` — If a window is provided, then mouse moves are only tracked when that window is the active (frontmost) window, and mouse positions are in Vuo Coordinates relative to the window. Otherwise, mouse moves are tracked whenever the composition is the active (frontmost) application, and mouse positions are in screen coordinates. In either case, mouse moves are tracked everywhere on the screen, including outside of the composition windows.
   - `Modifier Key` — The keyboard button (if any) that must be held for mouse moves to be tracked. 
   - `Moved To` — When the mouse is moved, fires an event with the position that it was moved to. 
