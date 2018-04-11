Outputs the values in one row of a table.

   - `Row` — The chosen row. This port's data type can be set to either Integer or Text.
      - Integer — 1 for the first row, 2 for the second row, etc. If less than 1, the first row is picked. If greater than the number of rows, the last row is picked.
      - Text — The header of the row. The first row whose header (first column value) contains the text (case-insensitive match) is picked. If no row matches or the text is empty, this node outputs an empty list.
   - `Include Header` — If *true*, the output includes the header for the selected row. If *false*, the output has one fewer item; the first item in the output comes from the second column of the table.
