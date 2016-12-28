Calculates the value of a math expression or formula. 

   - `Expression` — A math expression or formula, for example: 
      - `Width / 0,75` or `Width / 0.75`
      - `avg(A; B; C)` or `avg(A, B, C)`
      - `PI * R^2`
      - `Area = Width * Height`
   - `Values` — The numbers to substitute in place of the variables in `Expression`. 
   - `Result` — The number calculated from `Expression`. 

If your computer is set up for a region (such as most of Europe and South America) that uses a comma as the decimal mark, then you should also use commas (",") as decimal marks for this node, and you should use semicolons (";") to separate lists of numbers or variables. Otherwise (for regions such as China and the United States), you should use periods (".") as decimal marks, and you should use commas (",") to separate lists of numbers or variables.

The math expression may contain one or more *input variables* — names like `Distance`, `Time`, or `A` that are replaced with the numbers in `Values`. 

Optionally, the math expression may contain an *output variable* — a name for the calculated result, to help you remember what it represents. If there's an output variable, the math expression should have a `=` sign (assignment operator) with the output variable on the left and the input variables on the right. For example, in the expression `Area = Width * Height`, the output variable is `Area` and the input variables are `Width` and `Height`. 

The math expressions may contain: 

   - Numbers
   - Parentheses — for order of operations
      - `(` and `)` 
   - Constants — names that represent a certain number
      - `PI` — π, approximately 3.14
   - Operators
      - `+` — addition
      - `-` — subtraction
      - `*` — multiplication
      - `/` — division
      - `^` — power/exponent
      - `%` — modulus
      - `<` — less than
      - `>` — greater than
      - `<=` — less than or equal
      - `>=` - greater than or equal
      - `==` — equal
      - `!=` — not equal
      - `&&` — logical and
      - `||` — logical or
      - `?:` — if-then-else, for example: `Score > 100 ? Bonus : 0`
   - Functions
      - `avg` — average of a list of numbers
      - `sum` — sum of a list of numbers
      - `min` — minimum of a list of numbers
      - `max` — maximum of a list of numbers
      - `abs` — absolute value
      - `round` or `rint` — round to the nearest integer
      - `floor` / `ceil` — round to the nearest smaller/larger integer
      - `trunc` — round to the nearest integer with a smaller magnitude
      - `fract` — returns the fractional part
      - `clamp(x,min,max)` — constrains x to be &gt;= min and &lt;= max
      - `step(edge,x)` — returns 0 if x &lt; edge, or 1 if x &gt; edge
      - `smoothstep(edge0,edge1,x)` — returns a smooth 0 to 1 value as x changes from edge0 to edge1
      - `mix(x,y,t)` — linearly interpolate between x and y, at position t (0 to 1)
      - `sign` — -1 if negative, 1 if positive
      - `sqrt` — square root
      - `exp` — the mathematical constant *e* raised to a power
      - `ln` — logarithm to base *e*
      - `log` or `log10` — logarithm to base 10
      - `log2` — logarithm to base 2
      - `deg2rad` — convert degrees to radians
      - `rad2deg` — convert radians to degrees
      - `random(min,max)` — random number
      - Gradient noise
         - `perlin1d(x)`
         - `perlin2d(x,y)`
         - `perlin3d(x,y,z)`
         - `perlin4d(x,y,z,w)`
         - `simplex1d(x)`
         - `simplex2d(x,y)`
         - `simplex3d(x,y,z)`
         - `simplex4d(x,y,z,w)`
      - Trigonometry — for numbers in degrees
         - `sin`, `cos`, `tan`
         - `asin`, `acos`, `atan`, `atan2`
         - `sinh`, `cosh`, `tanh`
         - `asinh`, `acosh`, `atanh`
