Gives information about the parts of a file URL.

File URLs begin with `file:///` and refer to a file or folder on this computer.  (Other URLs, such as those that begin with `http://`, refer to network paths — see the `Get URL Values` node.)

   - `Path` — The full path of the file or folder the URL points to.
   - `Folder` — The full path of the folder the URL points to.  If the URL points to a file, the filename is excluded from this value (it includes everything up to, and including, the path's last slash).
   - `File Name` — The name of the file the URL points to, excluding the extension.  It includes just the part between the path's last slash and its last dot.
   - `Extension` — The file's extension (e.g., `jpg`).  It includes just the part after the path's last dot.
