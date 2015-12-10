Fires an event when the mouse is moved. 

   - `Window` — If a window is provided, then mouse actions are only tracked when that window is the active (frontmost) window, and mouse positions are in Vuo coordinates relative to the window. Otherwise, mouse actions are tracked while the running composition is active (frontmost), regardless of which of the composition's windows is active, and mouse positions are in screen coordinates. 
   - `Modifier Key` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `Moved To` — When the mouse is moved, fires an event with the position that it was moved to. 
