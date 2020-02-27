Adds text into other text.

`Position` is the location in the original text where the new text should be inserted.  The new text can be inserted at the beginning, at the end, or between characters in the original text.

If `Position` is 1 (or less), the new text is inserted at the beginning.  If `Position` is greater than the number of characters in the original text, then the new text is inserted at the end.

To always insert text at the end (no matter how long the original text is), use the [Append Texts](vuo-node://vuo.text.append) node.
