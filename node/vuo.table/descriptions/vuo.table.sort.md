Sorts a table's rows by the values in the chosen column.

   - `Column` — The chosen column. This port's data type can be set to either Integer or Text.
      - Integer — 1 for the first column, 2 for the second column, etc. If less than 1, the first column is picked. If greater than the number of columns, the last column is picked.
      - Text — The header of the column. The first column whose header (first row value) contains the text (case-insensitive match) is picked. If no column matches or the text is empty, this node outputs the input (unsorted) table.
   - `Sort Type` — How the values in the chosen column should be interpreted when sorting.
   - `Sort Order` — Ascending or descending.
   - `First Row Is Header` — If true, the first row is left in place while the rest of the table is sorted.

To sort on multiple columns, use multiple copies of this node. For example, to sort a school roster by class year and then by last name, set up one `Sort Table` node to sort by the Last Name column, and feed its output into another `Sort Table` node that sorts by the Class Year column.

To sort the table's columns instead of its rows, use the `Transpose Table` node before and after `Sort Table`.

If `Sort Type` is *Text (case-sensitive)*, uppercase letters come before lowercase letters in ascending order.

If `Sort Type` is *Date*, the column values can have any of the formats available in the `Format Date-Time` node, or any the "Short" formats from that node with a 2-digit instead of a 4-digit year.

If multiple items in the chosen column have the same value, this node keeps those rows in the same order as they were in the input table.
