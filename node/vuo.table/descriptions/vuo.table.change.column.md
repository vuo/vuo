Replaces the values in one column of a table.

   - `Column` — The chosen column. This port's data type can be set to either Integer or Text.
      - Integer — 1 for the first column, 2 for the second column, etc. If less than 1, the first column is picked. If greater than the number of columns, the last column is picked.
      - Text — The header of the column. The first column whose header (first row value) contains the text (case-insensitive match) is picked. If no column matches or the text is empty, this node outputs the original (unchanged) table.
   - `Preserve Header` — If *true*, the column's header is kept in place, and `New Values` replaces the rest of the column. If *false*, `New Values` replaces the entire column.
   - `New Values` — The replacement values, in order from first to last row. If there are fewer replacement values than rows in the table, the last rows in the modified table will contain empty text for this column. If there are more replacement values than rows in the table, the modified table will have rows added at the end, containing empty text for the other columns.
