Fires an event when a key is pressed or released. 

This node can be used to provide keyboard controls for a composition. The `Key` input port represents the key's position on the keyboard (*not* the character it types in the current keyboard layout). For example, if you create a composition using a QWERTY keyboard layout and set `Key` to *Q*, and you edit or run the composition using an AZERTY keyboard layout, then `Key` will automatically be translated to *A*. 

   - `Window` — If a window is provided, then keyboard buttons are only tracked when the window is active. Otherwise, keyboard buttons are tracked whenever the composition is the active (frontmost) application.
   - `Key` — The key(s) to track. 
   - `Modifier Key` — The modifier key (if any) that must be held for the key to be tracked. 
   - `Repeat when Held` — If *true*, the `Pressed` trigger port fires events repeatedly while the key is held down. If *false*, the `Pressed` trigger port only fires when the key is first pressed.
   - `Pressed` — When the key is pressed (or held, if `Repeat When Held` is *true*), fires an event. 
   - `Released` — When the key is released, fires an event. 
