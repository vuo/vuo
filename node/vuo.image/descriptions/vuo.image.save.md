Save an image to disk.

When an image is sent to the `saveImage` port, it is saved to the file path specified by `URL`.

- `URL` – The file path to save the image to.  For example, "MyImage.png" saves to directory that the current composition is in.  "~/Documents/MyOtherImage" saves to the `Home / Documents` folder (and automatically applies the extension).
- `overwriteUrl` – If true, an image already existing at `URL` will be overwritten with the new image.
- `format` – What file format to save the image as.  PNG is a good bet for images with transparency, and JPEG is good for small file sizes without transparency.
- `saveImage` – The image to be saved.