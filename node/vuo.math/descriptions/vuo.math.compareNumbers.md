Compares two numbers and returns the result.

Uses the provided `Comparison` operator to compare input value `A` to input value `B`. For example, if `A` is `4`, the `Comparison` operator is `>`, and `B` is `3`, evaluates the expression `4 > 3` and returns the result of `true`.

Because calculations on a computer are often inexact due to rounding errors, this node uses a small internal tolerance when evaluating the equality or inequality of real numbers. To specify a custom tolerance, use the `Are Equal` or `Are Not Equal` nodes.
