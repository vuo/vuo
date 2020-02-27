Fires an event when a mouse button is pressed or released. 

   - `Window` — If a window is provided, then mouse presses and releases are only tracked when the mouse was pressed on that window, and mouse positions are in Vuo Coordinates relative to the window. Otherwise, mouse presses and releases are tracked when the mouse was pressed on any window of the composition, and mouse positions are in screen coordinates.
   - `Button` — The mouse button(s) to track. 
   - `Modifier Key` — The keyboard button (if any) that must be held for mouse buttons to be tracked. 
   - `Pressed` — When the mouse button is pressed, fires an event with the position where it was pressed.
   - `Force Pressed` — On trackpads, when the button is force-pressed (pressed a normal amount, then beyond), fires an event with the position.
   - `Pressure Changed` — On trackpads, the force with which the button is being pressed.  Ranges from 0 (normal press), through 1 (force press), to 2 (maximum pressure).
   - `Released` — When the mouse button is released, fires an event with the position where it was released. 
