Creates a mesh in a shape defined by parametric math expressions.

This node creates a mesh whose vertex positions and texture mapping are defined by a set of math expressions. The expressions can use the variables *U* and *V*. This node calculates each expression for evenly spaced values of *U* and *V*. Each calculation becomes a vertex in the resulting mesh.

- `Time` — The time at which to calculate the expressions.  (This only affects the output if the `Time` variable is used in the expressions.)
- `X Expression`, `Y Expression`, `Z Expression` — The math expressions for the (X,Y,Z) position of each vertex. Typically, the values of `X Expression` should increase as *U* increases, and the values of `Y Expression` should increase as *V* increases. (Otherwise, the triangles that make up the mesh may be facing backwards, which means they won't be drawn.) 
- `Rows`, `Columns` — Each of the above expressions is calculated for `Rows` number of values of *V* and `Columns` number of values of *U*. The total number of vertices in the mesh is `Rows * Columns`.
- `U Closed`, `V Closed` — If *true*, then the mesh surface will wrap around from the vertices calculated for *U=1* or *V=1* to the vertices calculated for *U=0* or *V=0*. If *false*, then either the mesh surface won't wrap around or, if the math expressions do cause it to wrap around, any 3D scene object created from the mesh may have a visible seam. 

The math expressions can use:

   - Numbers
   - Operators such as *+* and *-*
   - Functions such as *sin(U)* and *log(U)*
   - Constants such as *PI*
   - Variables *U* and *V* — evenly spaced values ranging from `U Min` to `U Max` and `V Min` to `V Max`
   - Variables *I* and *J* — the integer index of the point, ranging from 1 to `Columns` and 1 to `Rows`, respectively

For a complete list of the operators and functions available, see the [Calculate](vuo-node://vuo.math.calculate) node's description.

For help understanding the math used by this node, see [Introduction to parametric equations](https://www.khanacademy.org/video/parametric-equations-1) and [Introduction to parametrizing a surface with two parameters](http://www.khanacademy.org/video/introduction-to-parametrizing-a-surface-with-two-parameters). 
