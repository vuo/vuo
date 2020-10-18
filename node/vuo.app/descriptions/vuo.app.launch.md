Launches the application from an `.app` file, or optionally brings it to the foreground if it's already running, when an event hits the `URL` port.

`URL` can be an absolute path or `file:` URL to the app bundle, or it can be a relative name (e.g., `Calculator.app`), in which case this node will look for the app in `/Users/<you>/Applications` and `/Applications`.  See [vuo.url](vuo-nodeset://vuo.url) for more info on how Vuo handles URLs.

If `Activate` is true, the application is brought to foreground focus.
