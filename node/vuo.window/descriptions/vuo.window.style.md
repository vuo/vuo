Changes the visual appearance of the window.

   - `Appearance` — Specifies whether the window's titlebar should be light or dark, to optionally override the system-wide setting.  (This has no effect on macOS 10.13 and earlier, since those macOS versions do not support dark mode.)
   - `Show Titlebar` — Whether the window shows a bar along the top, containing the title text, and the Close (red), Minimize (yellow), and Maximize (green) buttons.  If false, the bar will not be shown, which may make it difficult to manually move the window.
   - `Transparent Titlebar` — If true, the window's title text and Close/Minimize/Maximize buttons are composited onto the image rendered by the Vuo composition (rather than the default, which is a gray bar that's in a separate area from the composition's image).
   - `Show Shadow` — Whether the window has a drop shadow behind it.

These properties have no effect in full-screen mode.
