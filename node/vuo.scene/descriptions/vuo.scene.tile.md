Creates copies of a 3D object tiled in a grid.

   - `Object` — The object to copy.
   - `Center` — The position where the corners of the center 8 grid volumes touch, in Vuo Coordinates.
   - `Spacing` — The distance between the center of each tiled object, in Vuo Coordinates.
   - `Field Size` — The extent of the grid, in Vuo Coordinates.

When `Center` changes, objects that pass entirely outside the `Field Size` volume disappear, and reappear on the opposite side.  By animating `Center` and creating a field large enough to encompass the camera's field of view, you can create the illusion of traveling through an infinite series of objects.
