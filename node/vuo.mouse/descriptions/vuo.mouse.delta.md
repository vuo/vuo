Fires an event when the mouse is moved. 

   - `window` — If a window is provided, then mouse actions are only tracked when the window is active, and mouse positions are in Vuo coordinates relative to the window. Otherwise, mouse actions are tracked anywhere on the screen, and mouse positions are in screen coordinates. 
   - `modifierKey` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `movedBy` — When the mouse is moved, fires an event with the distance moved. This is the change in position since the previous `movedBy` event. If the pointer is already on the edge of the screen, the distance is non-zero even though the pointer stays in the same place on the screen. 
