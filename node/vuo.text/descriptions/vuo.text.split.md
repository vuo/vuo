Separates the text into parts.

   - `Separator` — The boundary between parts.
      - Or, if this contains no text (not even a space), the text is split into characters.
   - `Include Empty Parts` — How to handle consecutive separators or separators at the beginning or end of the text.
      - If *true*, an empty part is added to the output to represent the text between consecutive separators, before a separator at the beginning, or after a separator at the end.
      - If *false*, consecutive separators are treated as one, and separators at the beginning or end of the text are ignored.
