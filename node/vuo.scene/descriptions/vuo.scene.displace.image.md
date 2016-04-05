Translates each vertex in a direction perpendicular to its plane by an amount determined by the brightness of an image.

- `Object` – The scene object to displace.
- `Image` – The image to sample height values from.
- `Channel` – Which color channels to use when calculating the amount to displace a vertex.
- `Distance` – How far each vertex will be moved, in Vuo Coordinates.

When applying a mask using the `Channel` port, only the components specified will be used when calculating the displacement strength.  For example, if the `Channel` is set to `Red` and the pixel color is `RGBA (.5, 1, 1, 1)`, the resulting strength will be `.5`.
