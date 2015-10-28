Fires an event when a mouse button is clicked. 

   - `Window` — If a window is provided, then mouse actions are only tracked when the window is active, and mouse positions are in Vuo coordinates relative to the window. Otherwise, mouse actions are tracked anywhere on the screen, and mouse positions are in screen coordinates. 
   - `Button` — The mouse button(s) to track. 
   - `Modifier Key` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `Singled Clicked` — When the mouse button is single-clicked, fires an event with the position where it was clicked. 
   - `Double Clicked` — When the mouse button is double-clicked, fires an event with the position where it was clicked. 
   - `Triple Clicked` — When the mouse button is triple-clicked, fires an event with the position where it was clicked. 

When the mouse is double-clicked, only the `Double Clicked` port fires; the `Single Clicked` port doesn't also fire. Similarly, when the mouse is triple-clicked, only the `Triple Clicked` port fires; the `Single Clicked` and `Double Clicked` ports don't also fire. 
