Creates points that form a shape defined by parametric math expressions. 

The math expressions can use the variable *U*. This node calculates each expression for evenly spaced values of *U*. Each calculation becomes one of the resulting points.

- `Time` — The time at which to calculate the expressions.  (This only affects the output if the `Time` variable is used in the expressions.)
- `X Expression`, `Y Expression`, `Z Expression` — The math expressions for the (X,Y,Z) position of each point. 
- `Subdivisions` — Each of the above expressions is calculated for this many values of *U*. This is the total number of points created. 

The math expressions can use:

   - Numbers
   - Operators such as *+* and *-*
   - Functions such as *sin(U)* and *log(U)*
   - Constants such as *PI*
   - Variable *U* — evenly spaced values ranging from `U Min` to `U Max`
   - Variable *I* — the integer index of the point, ranging from 1 to `Subdivisions`

For a complete list of the operators and functions available, see the [Calculate](vuo-node://vuo.math.calculate) node's description.

For help understanding the math used by this node, see [Introduction to parametric equations](https://www.khanacademy.org/video/parametric-equations-1). 
