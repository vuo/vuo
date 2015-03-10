Fires an event when the mouse is dragged. 

   - `window` — If a window is provided, then mouse actions are only tracked when the window is active, and mouse positions are in Vuo coordinates relative to the window. Otherwise, mouse actions are tracked anywhere on the screen, and mouse positions are in screen coordinates. 
   - `button` — The mouse button(s) to track. 
   - `modifierKey` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `dragStarted` — When the mouse button is pressed, fires an event with the position where it was pressed. 
   - `dragMovedTo` — When the mouse is moved while the button is pressed, fires an event with the position that it was moved to. 
   - `dragEnded` — When the mouse button is released, fires an event with the position where it was released. 
