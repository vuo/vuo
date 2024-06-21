Saves an image to a file.

When an image is sent to the `Save Image` port, it is saved to the file path specified by a `URL`.

- `URL` – Where to save the image.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
- `Save Image` – When an image is received in this port, it will be saved to `URL` path.
- `If Exists` – What to do if a file already exists at `URL`:
   - `Don't Save` — The existing file will be left as-is; this node will not save the new image.
   - `Overwrite` — The existing file will be replaced with the new image.
   - `Append Number` — The image will be saved at the specified `URL` followed by a sequential number starting at 1.  (The existing file at the unmodified `URL` will be left as-is.)
   - `Append Timestamp` — The image will be saved at the specified `URL` followed by the current date and time.  (The existing file at the unmodified `URL` will be left as-is.  If a file already exists at the `URL` after the timestamp has been appended, it will be replaced with the new image.)
- `Format` – What file format to save the image as.
