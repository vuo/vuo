## Working with URL input ports

Many of Vuo's `Fetch` nodes take a URL as input.  These nodes can either load a file from the local system, or download a file from an Internet server.

To download a file from the Internet, copy the file's URL from your browser. Example:

   - `http://vuo.org/sites/all/themes/vuo_theme/logo.png`

To load a file on the computer running the composition, create a URL from the file's path. Examples: 

   - `file:///System/Library/CoreServices/DefaultDesktop.jpg`
   - `/System/Library/CoreServices/DefaultDesktop.jpg`
   - `DefaultDesktop.jpg` (for a file called "DefaultDesktop.jpg" in the same folder as the composition)
   - `images/DefaultDesktop.jpg` (for a file called "DefaultDesktop.jpg" in a folder called "images" in the same folder as the composition)

### Relative paths

If the file's location doesn't start with `file://` or `/`, then it's treated as a relative path. This is the option you'd typically want when sharing the composition with others. If running the composition on a different computer than it was created on, the file needs to exist in the same location relative to the composition (`.vuo`) file. If running an application exported from the composition, the file needs to exist in the `Contents/Resources` folder inside the application package; see the [Vuo Manual](http://vuo.org/manual.pdf) for details. 

If the composition hasn't been saved yet, paths are interpreted relative to the Desktop.

If you drag a file from Finder onto a URL port, Vuo will use a relative path to the file.

### Absolute paths

If the file's location starts with `file://` or `/`, then it's treated as an absolute path. If running the composition on a different computer than it was created on, the file needs to exist in the same location on that computer. To get the file's path in the correct format, open the TextEdit application, go to Format > Make Plain Text, drag the file onto the TextEdit window, and copy the path that appears in the TextEdit window. 

If you drag a file from Finder onto a URL port while holding <kbd>Option</kbd>, Vuo will use the file's absolute path.
