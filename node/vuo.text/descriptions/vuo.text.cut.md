Outputs part of the input text.

   - `text` — The original, whole text.
   - `startIndex` — The position of the first character in the part of `text` to output. 1 is the first character in `text`, 2 is the second character in `text`, etc.
   - `characterCount` — The number of characters in the part of `text` to output.

If `startIndex` is less than 1 or `characterCount` goes past the end of `text`, then the portion of `text` that falls within the range is output. For example, if `characterCount` is too large, then the part of `text` from `startIndex` to the end is output.

When finding the range of characters to output using `startIndex` and `characterCount`, this node counts each Unicode character in the text, including whitespace characters (spaces, tabs, line breaks).
