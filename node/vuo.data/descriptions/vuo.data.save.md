Saves data to a file.

When data is sent to the `Save Data` port, it is saved to the file path specified by a `URL`.

Use this to save information created by the [Convert Text to Data](vuo-node://vuo.type.text.data) or [Fetch Data](vuo-node://vuo.data.fetch) nodes.

- `URL` — Where to save the data.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
- `Save Data` – When data is received in this port, it will be saved to `URL` path.  If a file already exists, see below.
- `If Exists` – What to do if `URL` already exists:
   - Don't Save (leave the existing file intact)
   - Overwrite (replace the existing file with the new data)
   - Append (add the new data to the end of the existing file)
