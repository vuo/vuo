Creates points that form a shape defined by parametric math expressions. 

The math expressions can use the variable *u*. This node calculates each expression for evenly spaced values of *u* ranging from 0 to 1. Each calculation becomes one of the resulting points. 

- `Time` — The time at which to calculate the expressions.  (This only affects the output if the `time` variable (with a lower-case "t") is used in the expressions.)
- `X Expression`, `Y Expression`, `Z Expression` — The math expressions for the (X,Y,Z) position of each point. 
- `Subdivisions` — Each of the above expressions is calculated for this many values of *u*. This is the total number of points created. 

The math expressions can use numbers, operators such as *+* and *-*, functions such as *sin(u)* and *log(u)*, and the constant *PI*. For a complete list of the operators and functions available, see the `Calculate` (`vuo.math.calculate`) node's description. 

For help understanding the math used by this node, see [Introduction to parametric equations](https://www.khanacademy.org/video/parametric-equations-1). 
