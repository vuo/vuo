Nodes for breaking down a URL into its components.

## Working with URL input ports

Many of Vuo's `Fetch` nodes take a URL as input.  These nodes can either load a file from the local system, or download a file from an Internet server.

To download a file from the Internet, copy the file's URL from your browser. Example:

   - `https://vuo.org/sites/all/themes/vuo_theme/logo.png`

To load a file on the computer running the composition, create a URL from the file's path. Examples: 

   - `file:///System/Library/CoreServices/DefaultDesktop.jpg`
   - `/System/Library/CoreServices/DefaultDesktop.jpg`
   - `DefaultDesktop.jpg` (for a file called "DefaultDesktop.jpg" in the same folder as the composition)
   - `images/DefaultDesktop.jpg` (for a file called "DefaultDesktop.jpg" in a folder called "images" in the same folder as the composition)

### Relative paths

If the file's location doesn't start with `file://` or `/`, then it's treated as a relative path. This is the option you'd typically want when sharing the composition with others. If running the composition on a different computer than it was created on, the file needs to exist in the same location relative to the composition (`.vuo`) file. If running an application exported from the composition, the file needs to exist in the `Contents/Resources` folder inside the application package; see the [Vuo Manual](https://doc.vuo.org/latest/manual/exporting-an-application.xhtml) for details.

If the composition hasn't been saved yet, paths are interpreted relative to the Desktop.

If you drag a file from Finder onto a URL port, Vuo will use a relative path to the file.

### Absolute paths

If the file's location starts with `file://` or `/`, then it's treated as an absolute path. If running the composition on a different computer than it was created on, the file needs to exist in the same location on that computer. To get the file's path in the correct format, open the TextEdit application, go to Format > Make Plain Text, drag the file onto the TextEdit window, and copy the path that appears in the TextEdit window. 

If you drag a file from Finder onto a URL port while holding <kbd>Option</kbd>, Vuo will use the file's absolute path.

### Data URIs

You can embed images and other data directly into a Vuo composition (without requiring any external files or web servers) by using the `data:` URI scheme.  For example, if you paste this into the [Fetch Image](vuo-node://vuo.image.fetch) node's URL port:

   - `data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg==`

…it will output a red dot.

To convert a file into a Data URI, you can either use a web-based conversion tool such as [datauri.vuo.org](https://datauri.vuo.org), or launch Terminal.app and enter this command: `echo -n 'data:;base64,' ; base64 <path to image file>` then paste the output into one of Vuo's URL ports.
