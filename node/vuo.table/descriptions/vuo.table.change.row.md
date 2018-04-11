Replaces the values in one row of a table.

   - `Row` — The chosen row. This port's data type can be set to either Integer or Text.
      - Integer — 1 for the first row, 2 for the second row, etc. If less than 1, the first row is picked. If greater than the number of rows, the last row is picked.
      - Text — The header of the row. The first row whose header (first column value) contains the text (case-insensitive match) is picked. If no row matches or the text is empty, this node outputs the original (unchanged) table.
   - `Preserve Header` — If *true*, the row's header is kept in place, and `New Values` replaces the rest of the row. If *false*, `New Values` replaces the entire row.
   - `New Values` — The replacement values, in order from first to last column, including the header column if any. If there are fewer replacement values than columns in the table, the last columns in the modified table will contain empty text for this row. If there are more replacement values than columns in the table, the modified table will have columns added at the end, containing empty text for the other rows.
