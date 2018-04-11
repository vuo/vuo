Outputs the values in one column of a table.

   - `Column` — The chosen column. This port's data type can be set to either Integer or Text.
      - Integer — 1 for the first column, 2 for the second column, etc. If less than 1, the first column is picked. If greater than the number of columns, the last column is picked.
      - Text — The header of the column. The first column whose header (first row value) contains the text (case-insensitive match) is picked. If no column matches or the text is empty, this node outputs an empty list.
   - `Include Header` — If *true*, the output includes the header for the selected column. If *false*, the output has one fewer item; the first item in the output comes from the second row of the table.
