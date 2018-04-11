Applies a math expression to each value in a list, and outputs the new list.

   - `Expression` — A math expression or formula.
   - `X Values` — The numbers to substitute in place of the `X` variable in `Expression`.
   - `Constants` — The numbers to substitute in place of the other variables in `Expression`.
   - `Results` — The list of numbers calculated from `Expression`.

The math expression can use:

   - Numbers
   - Operators such as *+* and *-*
   - Functions such as *sin(X)* and *log(X)*
   - Constants such as *PI*
   - Variable *X* — the value of the current list item
   - Variable *I* — the integer index of the current list item, ranging from 1 to the number of items in the list

For a complete list of the operators and functions available, see the `Calculate` (`vuo.math.calculate`) node's description.
