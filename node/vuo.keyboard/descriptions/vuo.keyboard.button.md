Fires an event when a key is pressed or released. 

This node can be used to provide keyboard controls for a composition. The `key` input port represents the key's position on the keyboard (*not* the character it types in the current keyboard layout). For example, if you create a composition using a QWERTY keyboard layout and set `key` to *Q*, and you edit or run the composition using an AZERTY keyboard layout, then `key` will automatically be translated to *A*. 

   - `window` — If a window is provided, then keyboard buttons are only tracked when the window is active. Otherwise, keyboard buttons are tracked when any window in the composition is active. 
   - `key` — The key(s) to track. 
   - `modifierKey` — The modifier key (if any) that must be held for the key to be tracked. 
   - `repeatWhenHeld` — If *true*, the `pressed` trigger port fires events repeatedly while the key is held down. If *false*, the `pressed` trigger port only fires when the key is first pressed. 
   - `pressed` — When the key is pressed (or held, if `repeatWhenHeld` is *true*), fires an event. 
   - `released` — When the key is released, fires an event. 
