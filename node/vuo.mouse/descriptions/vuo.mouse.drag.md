Fires an event when the mouse is dragged. 

   - `Window` — If a window is provided, then mouse drags are only tracked if they started within that window, and mouse positions are in Vuo Coordinates relative to the window. Otherwise, mouse drags are tracked if they started within any window of the composition, and mouse positions are in screen coordinates.
   - `Button` — The mouse button(s) to track. 
   - `Modifier Key` — The keyboard button (if any) that must be held for mouse drags to be tracked. 
   - `Drag Started` — When the mouse button is pressed, fires an event with the position where it was pressed. 
   - `Drag Moved To` — When the mouse is moved while the button is pressed, fires an event with the position that it was moved to. 
   - `Drag Ended` — When the mouse button is released, fires an event with the position where it was released. 
