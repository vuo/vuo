Creates a mesh in a shape defined by parametric math expressions.

This node creates a mesh whose vertex positions and texture mapping are defined by a set of math expressions. The expressions can use the variables *u* and *v*. This node calculates each expression for evenly spaced values of *u* and *v* ranging from 0 to 1. Each calculation becomes a vertex in the resulting mesh. 

- `Time` — The time at which to calculate the expressions.  (This only affects the output if the `time` variable (with a lower-case "t") is used in the expressions.)
- `X Expression`, `Y Expression`, `Z Expression` — The math expressions for the (X,Y,Z) position of each vertex. Typically, the values of `X Expression` should increase as *u* increases, and the values of `Y Expression` should increase as *v* increases. (Otherwise, the triangles that make up the mesh may be facing backwards, which means they won't be drawn.) 
- `U Expression`, `V Expression` — The math expressions for the texture coordinate of each vertex. This defines how an image or other texture will be stretched across the mesh when the mesh is combined with a shader to create a 3D scene object. Typically, the values of these expressions should fall between 0 and 1, and should either always increase or always decrease, as *u* and *v* range from 0 to 1. 
- `Rows`, `Columns` — Each of the above expressions is calculated for `Rows` number of values of *u* and `Columns` number of values of *v*. The total number of vertices in the mesh is `rows X columns`. 
- `U Closed`, `V Closed` — If *true*, then the mesh surface will wrap around from the vertices calculated for *u=1* or *v=1* to the vertices calculated for *u=0* or *v=0*. If *false*, then either the mesh surface won't wrap around or, if the math expressions do cause it to wrap around, any 3D scene object created from the mesh may have a visible seam. 

The math expressions can use numbers, operators such as *+* and *-*, functions such as *sin(u)* and *log(u)*, and the constant *PI*. For a complete list of the operators and functions available, see the `Calculate` (`vuo.math.calculate`) node's description. 

For help understanding the math used by this node, see [Introduction to parametric equations](https://www.khanacademy.org/video/parametric-equations-1) and [Introduction to parametrizing a surface with two parameters](http://www.khanacademy.org/video/introduction-to-parametrizing-a-surface-with-two-parameters). 
