Displays a window that can show text and fire events when text is typed.

When the composition starts or this node is added to a running composition, it pops up a window that contains a text area.

   - `Write Line` — When this port receives an event, its text is written to the window, followed by a linebreak. The text is appended at the end of any text already in the window.
   - `Clear` — Removes all the text in the window.
   - `Typed Line` — When a linebreak is typed in the window, this port fires an event with the line just completed.
   - `Typed Word` — When a word boundary (whitespace following non-whitespace) is typed in the window, this port fires an event with the word just completed.
   - `Typed Character` — When a character is typed in the window, this port fires an event with the character just typed.
