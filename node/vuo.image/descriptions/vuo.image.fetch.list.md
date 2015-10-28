Loads or downloads images from a set of URLs.

If an image can't be loaded from one of the URLs, then it's skipped. The output list of images contains only the images that were successfully opened.

To download an image from the internet, copy the image's URL from your browser. Example: 

   - `http://vuo.org/sites/all/themes/vuo_theme/logo.png`

To load an image from a file on the computer running the composition, create a URL from the file's path. Examples: 

   - `file:///System/Library/CoreServices/DefaultDesktop.jpg`
   - `/System/Library/CoreServices/DefaultDesktop.jpg`
   - `DefaultDesktop.jpg` (for a file called "DefaultDesktop.jpg" in the same folder as the composition)
   - `images/DefaultDesktop.jpg` (for a file called "DefaultDesktop.jpg" in a folder called "images" in the same folder as the composition)

If the file's location doesn't start with `file://` or `/`, then it's treated as a relative path. This is the option you'd typically want when sharing the composition with others. If running the composition on a different computer than it was created on, the file needs to exist in the same location relative to the composition (`.vuo`) file. If running an application exported from the composition, the file needs to exist in the `Contents/Resources` folder inside the application package; see the [Vuo Manual](http://vuo.org/manual.pdf) for details. 

If the file's location starts with `file://` or `/`, then it's treated as an absolute path. If running the composition on a different computer than it was created on, the file needs to exist in the same location on that computer. To get the file's path in the correct format, open the TextEdit application, go to Format > Make Plain Text, drag the file onto the TextEdit window, and copy the path that appears in the TextEdit window. 
