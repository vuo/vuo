Calculates the value of a math expression or formula. 

   - `Expression` — A math expression or formula, for example: 
      - `distance / time`
      - `avg(a, b, c)`
      - `PI * r^2`
      - `area = width * height`
   - `Values` — The numbers to substitute in place of the variables in `Expression`. 
   - `Result` — The number calculated from `Expression`. 

The math expression may contain one or more *input variables* — names like `Distance`, `Time`, or `A` that are replaced with the numbers in `Values`. 

Optionally, the math expression may contain an *output variable* — a name for the calculated result, to help you remember what it represents. If there's an output variable, the math expression should have a `=` sign (assignment operator) with the output variable on the left and the input variables on the right. For example, in the expression `area = width * height`, the output variable is `Area` and the input variables are `Width` and `Height`. 

The math expressions may contain: 

   - Numbers
   - Parentheses — for order of operations
      - `(` and `)` 
   - Constants — names that represent a certain number
      - `P I` — π, approximately 3.14
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
      - `Avg` — average of a list of numbers
      - `Sum` — sum of a list of numbers
      - `Min` — minimum of a list of numbers
      - `Max` — maximum of a list of numbers
      - `Abs` — absolute value
      - `Rint` — round to the nearest integer
      - `Sign` — -1 if negative, 1 if positive
      - `Sqrt` — square root
      - `Exp` — the mathematical constant *e* raised to a power
      - `Ln` — logarithm to base *e*
      - `Log` or `Log 10` — logarithm to base 10
      - `Log 2` — logarithm to base 2
      - `Deg 2rad` — convert degrees to radians
      - `Rad 2deg` — convert radians to degrees
      - Trigonometry — for numbers in degrees
         - `Sin`, `Cos`, `Tan`
         - `Asin`, `Acos`, `Atan`
         - `Sinh`, `Cosh`, `Tanh`
         - `Asinh`, `Acosh`, `Atanh`
