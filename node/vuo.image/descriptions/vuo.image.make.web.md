Renders an image of a web page.

   - `Load URL` — Starts loading a web page from a URL.  You can use `http://`, `https://`, and `file:///` URLs.  Send another event to this port to reload the page.  When the page has finished loading, `Updated Image` will fire.
   - `Load HTML` — Starts loading raw HTML code.
   - `Re-render` — Fires another event from the `Updated Image` port.  Periodically send events to this port to show a web page's animations. (Limitation: This node is unable to play videos and animations on some macOS versions.)
   - `Scroll Mouse`, `Click Mouse` — Performs mouse actions on the web page.  Mouse positions are in Vuo Coordinates.
   - `Go Back`, `Go Forward` — Navigates through the web browser's page history.
   - `Width`, `Height` — The image's size, in pixels.

   - `Started Loading` — Fires the URL (if any) being loaded.
   - `Updated Progress` — Fires when the browser has made progress loading the page.  Ranges from 0 (not yet loaded) to 1 (fully loaded).
   - `Finished Loading` — Fires the URL (if any) when the page has finished loading.  The URL fired by `Finished Loading` may be different than `Started Loading` if the URL is redirected.
   - `Updated Image` — Fires the image of the web page.

If the output image is sent to a node that enlarges the image, then the text may become blurry. To keep the text sharp, either avoid using nodes that enlarge the image, or else choose a larger font size so that the output image is larger. For example, if creating a layer from the image, use the [Make Image Layer (Real Size)](vuo-node://vuo.layer.make.realSize2) node to keep the image at its original size.
