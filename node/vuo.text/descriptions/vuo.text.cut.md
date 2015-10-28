Outputs part of the input text.

   - `Text` — The original, whole text.
   - `Start Position` — The position of the first character in the part of `Text` to output. 1 is the first character in `Text`, 2 is the second character in `Text`, etc.
   - `Character Count` — The number of characters in the part of `Text` to output.

If `Start Position` is less than 1 or `Character Count` goes past the end of `Text`, then the portion of `Text` that falls within the range is output. For example, if `Character Count` is too large, then the part of `Text` from `Start Position` to the end is output.

When finding the range of characters to output using `Start Position` and `Character Count`, this node counts each Unicode character in the text, including whitespace characters (spaces, tabs, line breaks).
