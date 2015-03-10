Fires an event when a mouse button is clicked. 

   - `window` — If a window is provided, then mouse actions are only tracked when the window is active, and mouse positions are in Vuo coordinates relative to the window. Otherwise, mouse actions are tracked anywhere on the screen, and mouse positions are in screen coordinates. 
   - `button` — The mouse button(s) to track. 
   - `modifierKey` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `singledClicked` — When the mouse button is single-clicked, fires an event with the position where it was clicked. 
   - `doubleClicked` — When the mouse button is double-clicked, fires an event with the position where it was clicked. 
   - `tripleClicked` — When the mouse button is triple-clicked, fires an event with the position where it was clicked. 

When the mouse is double-clicked, only the `doubleClicked` port fires; the `singleClicked` port doesn't also fire. Similarly, when the mouse is triple-clicked, only the `tripleClicked` port fires; the `singleClicked` and `doubleClicked` ports don't also fire. 
