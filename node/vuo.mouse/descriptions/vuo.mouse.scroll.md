Fires an event when the mouse is scrolled. 

   - `Window` — If a window is provided, then mouse scroll actions are only tracked when the cursor is hovering above that window (regardless of which window is active / frontmost). Otherwise, mouse scroll actions are tracked when the cursor is hovering over any of the composition's windows.
   - `Modifier Key` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `Scrolled` — When the mouse is scrolled, fires an event with the distance scrolled. This is the change in position since the previous `Scrolled` event. The vertical distance (Y-coordinate) respects the operating system's "natural scrolling" setting. 

To track the cumulative distance the mouse has scrolled along an axis, connect the output of this node to a `Count` node.
