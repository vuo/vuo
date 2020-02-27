Creates a graphics shader that can paint the outline of a 3D object.

This shader ignores lighting.

   - `Front Color` — The color of the object's outline, for front-facing triangles.
   - `Back Color` — The color of the object's outline, for back-facing triangles.  This only has an effect if back-face rendering is enabled (see the [Show Back of 3D Object](vuo-node://vuo.scene.back) node).
   - `Width` — How thick the outline should be, specified as the percentage of a full triangle — 0 results in an invisible outline; 1 fills in the entire object.
   - `Standoff Width` — The extra triangle percentage to paint black, beyond the outline width.  This is useful in combination with the [Show Back of 3D Object](vuo-node://vuo.scene.back) node — when showing both the front and back of an object, a nonzero `Standoff Width` can make the object look like a schematic rendering, where lines in the background avoid interecting with lines in the foreground.
   - `Show Third Edge` — Whether to draw all 3 edges of each triangle, or only 2 of them.  In computer graphics, rectangles are drawn using a pair of triangles.  If all 3 edges of a triangle are drawn, the rectangle will have a diagonal line through it.  Vuo's mesh nodes order the triangle edges so that the diagonal is the third edge, so that, by setting this port value to false, rectangles can be rendered without the diagonal.
   - `Uniform Width` — Whether to draw all lines at the same width, or to scale depending on the size of the triangle. If `true`, then when rendering a triangular structure, you should turn `Show Third Edge` on, since edges will look inconsistent otherwise. (At some seams, lines will be drawn for both adjacent triangles, whereas at other seams, only one or no lines may be drawn, depending on how the triangles are oriented.)
