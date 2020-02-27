Gives a list of all files in a folder.

   - `Folder` — The path or link to a folder on the computer running the composition. If this is left blank, then the folder that contains the composition is used.
   - `Include Subfolders` — If *true*, any folders inside of `Folder` (and any folders inside of them, and so on) are checked for files. If *false*, this node will only output the files in the top level of `Folder`.
   - `File Type` — The kind of files to search for. 
      - "Any File" — All files (not including folders).
      - "Audio" — Audio files, such as .wav or .mp3.
      - "Image" — Image files, such as .png or .jpg.
      - "Mesh" — Mesh (.data) files.
      - "Movie" — Movie files, such as .mov or .h264.
      - "Scene" — 3D scene files, such as .3ds or .dae.
      - "Folder" — Folders only (no files).
   - `Files` — The files found, in case-sensitive lexicographic order — file names that start with numbers are first, followed by ones that start with uppercase letters, followed by ones that start with lowercase letters. Hidden files (those that start with ".") are not included.

The easiest way to set `Folder` (if not leaving it blank) is to drag the folder from Finder onto the constant flag next to the port.

The `Folder` can be either a path or a `file://` URL. Examples: 

   - `file:///Library/Desktop Pictures`
   - `/Library/Desktop Pictures`
   - `Images` (for a folder called "images" in the same folder as the composition)
   - `images/nature` (for a folder called "nature" in a folder called "images" in the same folder as the composition)

If the folder's location doesn't start with `file://` or `/`, then it's treated as a relative path. This is the option you'd typically want when sharing the composition with others. If running the composition on a different computer than it was created on, the folder needs to exist in the same location relative to the composition (`.vuo`) file. If running an application exported from the composition, the folder needs to exist in the `Contents/Resources` folder inside the application package; see the [Vuo Manual](https://vuo.org/manual.pdf) for details.

If the folder's location starts with `file://` or `/`, then it's treated as an absolute path. If running the composition on a different computer than it was created on, the folder needs to exist in the same location on that computer.
