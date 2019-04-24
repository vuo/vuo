Creates a scene object using the "supershape" formula.

The "supershape" formula ("superformula") is a way to create natural-looking objects.  By smoothly changing the parameters, you can morph between a wide variety of shapes.

- `Transform` — Changes the shape's translation, rotation, or scale, using Vuo Coordinates.
- `Material` — A shader, image, or color to apply to the shape.
- `m` — The number of lobes the shape has.  For example, 0 produces a round or spherical object, and 6 produces a hexagonal solid.
- `n1`, `n2`, and `n3` — The shape's exponents.  Values less than 1 tend to produce pinched shapes, and larger values tend to produce more relaxed shapes.
- `a` and `b` — The length of the shape's major and minor axes.
- `Angle` — The angles to begin and end at, in degrees counterclockwise around the Y axis.
- `Radius` — The radius to use at the beginning and ending angle.
- `Rows` and `Columns` — The number of vertices along the shape's latitude/longitude. With more vertices, the shape will be smoother but may take longer to render.

[Paul Bourke's page](http://paulbourke.net/geometry/supershape/) provides additional information and example parameters for generating a variety of shapes.
