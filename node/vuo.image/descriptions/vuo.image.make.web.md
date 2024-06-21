Renders an image of a web page.

   - `Load URL` — Starts loading a web page from a URL.  Send another event to this port to reload the page.  When the page has finished loading, `Updated Image` will fire.
      - For a web page on the Internet, the URL may begin with `https://` or `http://` (or neither, in which case it defaults to `http://`).
      - For a web page on your computer, the URL must begin with `file://` followed by an absolute path.  See the section below if you need to use a relative path.  See [vuo.url](vuo-nodeset://vuo.url) for an explanation of absolute and relative paths.
   - `Load HTML` — Starts loading raw HTML code.
   - `Re-render` — Fires another event from the `Updated Image` port.  Periodically send events to this port to show a web page's animations. (Limitation: This node is unable to play videos and animations on some macOS versions.)
   - `Scroll Mouse`, `Click Mouse` — Performs mouse actions on the web page.  Mouse positions are in Vuo Coordinates.
   - `Go Back`, `Go Forward` — Navigates through the web browser's page history.
   - `Width`, `Height` — The image's size, in pixels.

   - `Started Loading` — Fires the URL (if any) being loaded.
   - `Updated Progress` — Fires when the browser has made progress loading the page.  Ranges from 0 (not yet loaded) to 1 (fully loaded).
   - `Finished Loading` — Fires the URL (if any) when the page has finished loading.  The URL fired by `Finished Loading` may be different than `Started Loading` if the URL is redirected.
   - `Updated Image` — Fires the image of the web page.

### Resizing the output image

If the output image is sent to a node that enlarges the image, then the text may become blurry. To keep the text sharp, either avoid using nodes that enlarge the image, or else choose a larger font size so that the output image is larger. For example, if creating a layer from the image, use the [Make Image Layer (Real Size)](vuo-node://vuo.layer.make.realSize2) node to keep the image at its original size.

### Using a relative file path

The `Load URL` port doesn't accept relative file paths directly. This means you can't simply type in a file name or drag and drop a file from Finder onto the port. Instead, you can do one of the following:

   - Hold `⌥` (Option) while dragging and dropping the file onto the `Load URL` port. This will fill in an absolute path.
   - Use [List Files](vuo-node://vuo.file.list) (with its `Folder` port left empty) and [Get File URL Values](vuo-node://vuo.url.get.file) to get the absolute path of the folder containing the composition. Append the file's relative path to that to construct an absolute path.
   - Use [Fetch Data](vuo-node://vuo.data.fetch) and [Convert Data to Text](vuo-node://vuo.type.data.text) to load the file contents from a relative path and feed them into the `Load HTML` port.
