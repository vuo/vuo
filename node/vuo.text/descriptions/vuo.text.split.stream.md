Separates a stream of text into parts.

This node is useful for receiving a series of text pieces (such as keyboard typing) and splitting it into parts (such as comma-delimited items). Each time a separator is encountered in the text, this node fires an event with the text part just before the separator.

   - `Text` — The most recent text in the stream. Send a series of text to this port.
   - `Separator` — The boundary between parts.
      - Or, if this contains no text (not even a space), the text is split into characters.
   - `Include Empty Parts` — How to handle consecutive separators or a separator at the beginning of the text.
      - If *true*, an empty part is added to the output to represent the text between consecutive separators or before a separator at the beginning.
      - If *false*, consecutive separators are treated as one, and separators at the beginning of the text are ignored.
