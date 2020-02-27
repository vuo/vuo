Nodes for manipulating and getting information from text.

Almost all Vuo nodes use the UTF-8 encoding for the Unicode character set.  This means that they can handle ASCII characters (such as the English/Latin alphabet) as well as others, such as accented characters, non-Latin alphabets, symbols, and emoji.  Nodes with `ASCII` in the title use just the [ASCII character set](https://en.wikipedia.org/wiki/ASCII) (not full Unicode).

You can enter multiple lines of text into a text input editor by using Option-Return to insert line breaks.

Several nodes in this set — such as [Cut Text](vuo-node://vuo.text.cut), [Insert Text](vuo-node://vuo.text.insert), and [Find Text](vuo-node://vuo.text.find) — use an index number to represent a position in the text.  Index 1 represents the first character.

## Comparing text
Some nodes, such as [Compare Texts](vuo-node://vuo.text.compare) and [Find Subtrees with Content](vuo-node://vuo.tree.find.content), provide multiple methods for comparing text:

   - **Comparison type**
      - **Equals** — The entire subject text must match the entire search text.
      - **Contains** — The subject text must contain the search text.
      - **Begins with** — The subject text must start with the search text.
      - **Ends with** — The subject text must end with the search text.
      - **Matches wildcard** — The entire subject text must match the wildcard text, which can use special symbols to define the matching criteria:
         - `?` matches any single character
         - `*` matches anything — no characters, or one or more characters
         - `[xyz]` matches one of the characters between the brackets
         - See [glob(7)](http://man7.org/linux/man-pages/man7/glob.7.html#DESCRIPTION) for more information.
      - **Matches regular expression** — The subject text must match the regular expression, which can use special symbols to define the matching criteria:
         - `.` matches any single character
         - `.+` matches one or more characters
         - `.*` matches anything — no characters, or one or more characters
         - `[xyz]` matches one of the characters between the brackets
         - `[xyz]+` matches _one or more_ of characters between the brackets
         - The regular expression does not need to match the _entire_ subject text, though by using anchors (`^…$`) you can require that.
         - See [this introduction](https://ryanstutorials.net/regular-expressions-tutorial/regular-expressions-basics.php) for more information.  This comparison type uses [POSIX Extended Regular Expressions](http://man7.org/linux/man-pages/man7/regex.7.html#DESCRIPTION).
   - **Case sensitivity** — When enabled, the capitalization of the texts must match.
