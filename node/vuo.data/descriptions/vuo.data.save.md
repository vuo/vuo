Saves data to a file.

When data is sent to the `Save Data` port, it is saved to the file path specified by a `URL`.

Use this to save information created by the `Convert Text to Data` or `Fetch Data` nodes.

- `URL` — Where to save the data.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
- `Save Data` – When data is received in this port, it will be saved to `URL` path.  If a file already exists at the path, it will not be saved unless `Overwrite URL` is set to true.
- `Overwrite URL` – If true, a file already existing at the `URL` will be overwritten with the new file.
