Creates points that form a shape defined by parametric math expressions. 

The math expressions can use the variables *U* and *V*. This node calculates each expression for evenly spaced values of *U* and *V*. Each calculation becomes one of the resulting points.

- `Time` — The time at which to calculate the expressions.  (This only affects the output if the `Time` variable is used in the expressions.)
- `X Expression`, `Y Expression`, `Z Expression` — The math expressions for the (X,Y,Z) position of each point. 
- `Rows`, `Columns` — Each of the above expressions is calculated for `Rows` number of values of *V* and `Columns` number of values of *U*. The total number of points created is `Rows * Columns`.

The math expressions can use:

   - Numbers
   - Operators such as *+* and *-*
   - Functions such as *sin(U)* and *log(U)*
   - Constants such as *PI*
   - Variables *U* and *V* — evenly spaced values ranging from `U Min` to `U Max` and `V Min` to `V Max`
   - Variables *I* and *J* — the integer index of the point, ranging from 1 to `Columns` and 1 to `Rows`, respectively

For a complete list of the operators and functions available, see the [Calculate](vuo-node://vuo.math.calculate) node's description.

For help understanding the math used by this node, see [Introduction to parametric equations](https://www.khanacademy.org/video/parametric-equations-1) and [Introduction to parametrizing a surface with two parameters](http://www.khanacademy.org/video/introduction-to-parametrizing-a-surface-with-two-parameters). 
