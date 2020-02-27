Fires an event when the mouse is scrolled. 

   - `Window` — If a window is provided, then mouse scrolls are only tracked when the cursor is hovering above that window. Otherwise, mouse scrolls are tracked when the cursor is hovering over any of the composition's windows. In either case, mouse scrolls are tracked regardless of whether the window is the active (frontmost) window and the composition is the active (frontmost) application.
   - `Modifier Key` — The keyboard button (if any) that must be held for the mouse scrolls to be tracked. 
   - `Scrolled` — When the mouse is scrolled, fires an event with the distance scrolled. This is the change in position since the previous `Scrolled` event. The vertical distance (Y-coordinate) respects the operating system's "natural scrolling" setting. 

To track the cumulative distance the mouse has scrolled along an axis, connect the output of this node to a `Count` node.
