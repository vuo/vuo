Shows a window allowing the user to select a folder and enter a file name for saving a file.

   - `Show` — Causes the window to appear.
   - `Title` — The text at the top of the window.
   - `Field Label` — The text left of the file name field.
   - `Default File Name` — The initial text in the file name field.
   - `Button Label` — The text on the button in the lower-right corner of the window.
   - `File Type` — The type of file to allow saving.  The value of this field populates the default file extension — for example, the Image type adds the `.png` extension.
   - `Default URL` — The window's starting folder and (optionally) file name.  If this value includes a file name, the file will be highlighted in the file window, and its name will appear in the file name field.
   - `Parent Window` — If provided, the Save Window appears as a sheet attached to the Parent Window.  If not provided, the Save Window appears as an independent window.
   - `Selected URL` — The selected path and file name.  (If the user clicks Cancel, the event is blocked.)
   - `Canceled` — Outputs an event if the user clicks Cancel in the dialog.
