Shows a window allowing the user to select one or more files or folders.

   - `Show` — Causes the window to appear.
   - `Title` — The text at the top of the window.
   - `Button Label` — The text on the button in the lower-right corner of the window.
   - `File Type` — Which kind of files (or folders) are allowed to be selected.
   - `Default URL` — The window's starting folder.
   - `Select Multiple` — Whether the window allows selecting multiple files/folders, or just one.
   - `Parent Window` — If provided, the Open Window appears as a sheet attached to the Parent Window.  If not provided, the Open Window appears as an independent window.
   - `Selected URLs` — The selected files/folders.  (If the user clicks Cancel, the event is blocked.)
   - `Canceled` — Outputs an event if the user clicks Cancel in the dialog.
