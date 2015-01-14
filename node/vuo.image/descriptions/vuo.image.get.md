Loads or downloads an image from a URL.

To download an image from the internet, copy the image's URL from your browser. Example: 

   - `http://vuo.org/sites/all/themes/vuo_theme/logo.png`

Currently, URLs that begin with `https` are not supported.

To load an image from a file on the computer running the composition, create a URL from the image file's path. Examples: 

   - `file:///System/Library/CoreServices/DefaultDesktop.jpg`
   - `file:///Users/me/file\ with\ spaces.png` (for a file called `file with spaces.png`)

To get a file's path in the correct format, open the Terminal application, drag the file onto the Terminal window, and copy the path that appears in the Terminal window. Add `file://` at the beginning of the path.

Currently, only absolute file paths are supported. If you run a composition on a different computer than it was created on, then the image files need to be in the same location as on the original computer. 
