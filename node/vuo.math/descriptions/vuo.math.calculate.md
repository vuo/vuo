Calculates the value of a math expression or formula. 

   - `expression` — A math expression or formula, for example: 
      - `distance / time`
      - `avg(a, b, c)`
      - `PI * r^2`
      - `area = width * height`
   - `values` — The numbers to substitute in place of the variables in `expression`. 
   - `result` — The number calculated from `expression`. 

The math expression may contain one or more *input variables* — names like `distance`, `time`, or `a` that are replaced with the numbers in `values`. 

Optionally, the math expression may contain an *output variable* — a name for the calculated result, to help you remember what it represents. If there's an output variable, the math expression should have a `=` sign (assignment operator) with the output variable on the left and the input variables on the right. For example, in the expression `area = width * height`, the output variable is `area` and the input variables are `width` and `height`. 

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
      - `?:` — if-then-else, for example: `score > 100 ? bonus : 0`
   - Functions
      - `avg` — average of a list of numbers
      - `sum` — sum of a list of numbers
      - `min` — minimum of a list of numbers
      - `max` — maximum of a list of numbers
      - `abs` — absolute value
      - `rint` — round to the nearest integer
      - `sign` — -1 if negative, 1 if positive
      - `sqrt` — square root
      - `exp` — the mathematical constant *e* raised to a power
      - `ln` — logarithm to base *e*
      - `log` or `log10` — logarithm to base 10
      - `log2` — logarithm to base 2
      - `deg2rad` — convert degrees to radians
      - `rad2deg` — convert radians to degrees
      - Trigonometry — for numbers in degrees
         - `sin`, `cos`, `tan`
         - `asin`, `acos`, `atan`
         - `sinh`, `cosh`, `tanh`
         - `asinh`, `acosh`, `atanh`
