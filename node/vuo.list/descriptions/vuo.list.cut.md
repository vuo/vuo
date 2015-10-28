Outputs part of the input list.

   - `List` — The original, whole list.
   - `Start Position` — The position of the first item in the part of `List` to output. 1 is the first item in `List`, 2 is the second item in `List`, etc.
   - `Item Count` — The number of items in the part of `List` to output.

If `Start Position` is less than 1 or `Item Count` goes past the end of `List`, then the portion of `List` that falls within the range is output. For example, if `Item Count` is too large, then the part of `List` from `Start Position` to the end is output.
