Fires an event when the mouse is dragged. 

   - `Window` — If a window is provided, then mouse actions are only tracked when that window is the active (frontmost) window, and mouse positions are in Vuo coordinates relative to the window. Otherwise, mouse actions are tracked while the running composition is active (frontmost), regardless of which of the composition's windows is active, and mouse positions are in screen coordinates. 
   - `Button` — The mouse button(s) to track. 
   - `Modifier Key` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `Drag Started` — When the mouse button is pressed, fires an event with the position where it was pressed. 
   - `Drag Moved To` — When the mouse is moved while the button is pressed, fires an event with the position that it was moved to. 
   - `Drag Ended` — When the mouse button is released, fires an event with the position where it was released. 
