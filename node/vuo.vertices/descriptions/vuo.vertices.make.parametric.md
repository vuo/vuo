Creates a mesh in a shape defined by parametric math expressions.

This node creates a mesh whose vertex positions and texture mapping are defined by a set of math expressions. The expressions can use the variables *u* and *v*. This node calculates each expression for evenly-spaced values of *u* and *v* ranging from 0 to 1. Each calculation becomes a vertex in the resulting mesh. 

- `time` — The time at which to calculate the expressions.  (This only affects the output if the `time` variable is used in the expressions.)
- `xExpression`, `yExpression`, `zExpression` — The math expressions for the (x,y,z) position of each vertex. Typically, the values of `xExpression` should increase as *u* increases, and the values of `yExpression` should increase as *v* increases. (Otherwise, the triangles that make up the mesh may be facing backwards, which means they won't be drawn.) 
- `uExpression`, `vExpression` — The math expressions for the texture coordinate of each vertex. This defines how an image or other texture will be stretched across the mesh when the mesh is combined with a shader to create a 3D scene object. Typically, the values of these expressions should fall between 0 and 1, and should either always increase or always decrease, as *u* and *v* range from 0 to 1. 
- `rows`, `columns` — Each of the above expressions is calculated for `rows` number of values of *u* and `columns` number of values of *v*. The total number of vertices in the mesh is `rows x columns`. 
- `uClosed`, `vClosed` — If *true*, then the mesh surface will wrap around from the vertices calculated for *u=1* or *v=1* to the vertices calculated for *u=0* or *v=0*. If *false*, then either the mesh surface won't wrap around or, if the math expressions do cause it to wrap around, any 3D scene object created from the mesh may have a visible seam. 

The math expressions can use numbers, [operators](http://muparser.beltoforion.de/mup_features.html#idDef3) such as *+* and *-*, [functions](http://muparser.beltoforion.de/mup_features.html#idDef2) such as *sin(u)* and *log(u)*, and the constants *PI*, *DEG2RAD*, and *RAD2DEG*. 

For help understanding the math used by this node, see [Introduction to parametric equations](https://www.khanacademy.org/video/parametric-equations-1) and [Introduction to parametrizing a surface with two parameters](http://www.khanacademy.org/video/introduction-to-parametrizing-a-surface-with-two-parameters). 
