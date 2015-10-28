Fires an event when a mouse button is pressed or released. 

   - `Window` — If a window is provided, then mouse actions are only tracked when the window is active, and mouse positions are in Vuo coordinates relative to the window. Otherwise, mouse actions are tracked anywhere on the screen, and mouse positions are in screen coordinates. 
   - `Button` — The mouse button(s) to track. 
   - `Modifier Key` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `Pressed` — When the mouse button is pressed, fires an event with the position where it was pressed. 
   - `Released` — When the mouse button is released, fires an event with the position where it was released. 
