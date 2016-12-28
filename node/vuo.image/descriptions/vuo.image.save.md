Save an image to a file.

When an image is sent to the `Save Image` port, it is saved to the file path specified by a `URL`.

- `URL` – Where to save the image.  See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.
- `Save Image` – When an image is received in this port, it will be saved to `URL` path.  If a file already exists at the path, it will not be saved unless `Overwrite URL` is set to true.
- `Overwrite URL` – If true, an image already existing at the `URL` will be overwritten with the new image.
- `Format` – What file format to save the image as.
