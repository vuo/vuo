Opens a document file in the default app for that document, launching the app if necessary, when an event hits the `URL` port.

For example:

   - You can open a document by using its filename, like `~/Desktop/filename.rtf`
   - You can open a Mail message by using a URL like `message://<…>` (View > Message > Raw Source, and copy the "Message-Id" header)
   - You can start composing a new Mail message by using a URL like `mailto:user@example.com`
   - You can open the Maps app by using a URL like `http://maps.apple.com/?q=park`
   - You can initiate a FaceTime call by using a URL like `facetime://user@example.com` or `tel:1-408-555-5555`
   - You can open an [Evernote note](https://dev.evernote.com/doc/articles/note_links.php) by using a URL like `evernote:///view/…` (right-click, then hold Option and click "Copy Note Link")

See the [Apple URL Schemes documentation](https://developer.apple.com/library/content/featuredarticles/iPhoneURLScheme_Reference/) for more examples.

If `Activate` is true, the application is brought to foreground focus.

See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
