Requests that the computer (and its display) refrain from automatically going to sleep, and deactivates the screen saver.

When the `Keep System Awake` port is _true_ and this node receives an event, the system will no longer automatically show the screen saver or go sleep when idle (when the keyboard and mouse haven't been used for a few minutes, depending on your System Settings > Lock Screen settings).

When the composition stops, or when the `Keep System Awake` port is _false_ and this node receives an event, the system returns to its default settings.

Regardless, the system will still go to sleep when forced (when you select Apple > Sleep from the menu bar, or when you close the lid of a MacBook).
