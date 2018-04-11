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

The math expression may also contain:

   - Numbers
   - Constants — names that represent a certain number
      - `PI` — π, approximately 3.14
   - Parentheses — for order of operations
      - `(` and `)` 
   - Operators
      - `a + b` — addition
      - `a - b` — subtraction
      - `a * b` — multiplication
      - `a / b` — division
      - `a^b` — power/exponent
      - `a % b` — modulus
      - `a < b` — less than
      - `a > b` — greater than
      - `a <= b` — less than or equal
      - `a >= b` - greater than or equal
      - `a == b` — equal
      - `a != b` — not equal
      - `a && b` — logical and
      - `a || b` — logical or
      - `a ? b : c` — if-then-else, for example: `Score > 100 ? Bonus : 0`
   - Functions
      - `sqrt(n)` — square root
      - `exp(n)` — the mathematical constant *e* raised to a power
      - `ln(n)` — logarithm to base *e*
      - `log(n)` or `log10(n)` — logarithm to base 10
      - `log2(n)` — logarithm to base 2
      - `round(n)` or `rint(n)` — round to the nearest integer
      - `floor(n)` / `ceil(n)` — round to the nearest smaller/larger integer
      - `trunc(n)` — round to the nearest integer with a smaller magnitude
      - `fract(n)` — returns the fractional part
      - `abs(n)` — absolute value
      - `sign(n)` — -1 if negative, 1 if positive
      - `deg2rad(n)` — convert degrees to radians
      - `rad2deg(n)` — convert radians to degrees
      - `avg(a,b,…)` — average of a list of numbers
      - `sum(a,b,…)` — sum of a list of numbers
      - `min(a,b,…)` — minimum of a list of numbers
      - `max(a,b,…)` — maximum of a list of numbers
      - `random(min,max)` — random number
      - `clamp(n,min,max)` — constrains n to be &gt;= min and &lt;= max
      - `step(edge,n)` — returns 0 if n &lt; edge, or 1 if n &gt; edge
      - `smoothstep(edge0,edge1,n)` — returns a smooth 0 to 1 value as n changes from edge0 to edge1
      - `mix(a,b,t)` — linearly interpolate between a and b, at position t (0 to 1)
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
         - `sin(n)`, `cos(n)`, `tan(n)`
         - `asin(n)`, `acos(n)`, `atan(n)`, `atan2(y,x)`
         - `sinh(n)`, `cosh(n)`, `tanh(n)`
         - `asinh(n)`, `acosh(n)`, `atanh(n)`
