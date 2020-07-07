Outputs *true* if all values are close to equal.

If there are no values, outputs *true*.

`Tolerance` is the maximum amount by which any two values can differ to still be considered equal. Because calculations on a computer are often inexact due to rounding errors, if working with real numbers, you would typically want to make `Tolerance` greater than 0.

For point types, this node checks whether each component of each value is equal within each component of `Tolerance`.
