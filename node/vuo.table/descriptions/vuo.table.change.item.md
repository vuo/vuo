Replaces an item in a table.

   - `Row` — The chosen row. This port's data type can be set to either Integer or Text.
      - Integer — 1 for the first row, 2 for the second row, etc. If less than 1, the first row is picked. If greater than the number of rows, the last row is picked.
      - Text — The header of the row. The first row whose header (first column value) contains the text (case-insensitive match) is picked. If no row matches or the text is empty, this node outputs the original (unchanged) table.
   - `Column` — The chosen column, similar to `Row`.
