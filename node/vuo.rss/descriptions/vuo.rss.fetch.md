Loads or downloads an RSS feed from a URL.

   - `URL` — The URL of an RSS feed.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
   - `Fetch Images` — Whether to load or download an image associated with each feed item.  Depending on the connection speed and quantity and size of the images, this can take a substantial amount of time.  In either case, if the feed item provides an image, you can get its URL from the `Get RSS Item Values` node's `Image URL` port and download it yourself.
