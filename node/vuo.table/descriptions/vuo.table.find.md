Finds the first matching item in a table, and outputs the entire table row containing that value.

   - `Column` — Which column to search. This port's data type can be set to either Integer or Text.
	  - Integer — 1 for the first column, 2 for the second column, etc. If less than 1, the first column is picked. If greater than the number of columns, the last column is picked.
	  - Text — The header of the column. The first column whose header (first row value) contains the text (case-insensitive match) is picked. If no column matches or the text is empty, this node outputs an empty list.
   - `Value` — Which item to search for.
   - `Value Comparison` — How to determine whether each item equals the specified `Value`.  For example, if `Value Comparison` is "begins with", this node searches for an item whose value begins with `Value`.  See the [Text node set documentation](vuo-nodeset://vuo.text) for more information.
   - `Include Header` — If *true*, the output includes the header for the selected row. If *false*, the output has one fewer item; the first item in the output comes from the second column of the table.
