Nodes for working with information that has a tabular structure made up of rows and columns, like a spreadsheet.

You can import a spreadsheet into a Vuo composition by exporting it from your spreadsheet application in [CSV (Comma-Separated Values)](https://en.wikipedia.org/wiki/Comma-separated_values) or [TSV (Tab-Separated Values)](https://en.wikipedia.org/wiki/Tab-separated_values) format and using the [Fetch Data](vuo-node://vuo.data.fetch) and [Make Table from Text](vuo-node://vuo.table.make.text) nodes. You can export CSV or TSV data from a Vuo composition, to be imported into your spreadsheet application, by using the [Format Table](vuo-node://vuo.table.format) and [Save Data](vuo-node://vuo.data.save) nodes.

Like a spreadsheet, Vuo's Table data type stores information in a grid of rows and columns. For example, here's a table of top-ranking tennis players <small>(source: http://www.wtatennis.com/rankings)</small>:

<table>
   <tr><td>Rank</td><td>Player</td><td>Country</td><td>Points</td></tr>
   <tr><td>1</td><td>Serena Williams</td><td>United States</td><td>7010</td></tr>
   <tr><td>2</td><td>Angelique Kerber</td><td>Germany</td><td>6925</td></tr>
   <tr><td>3</td><td>Karolina Pliskova</td><td>Czech Republic</td><td>6020</td></tr>
   <tr><td>4</td><td>Dominika Cibulkova</td><td>Slovak Republic</td><td>5065</td></tr>
   <tr><td>5</td><td>Simona Halep</td><td>Romania</td><td>5021</td></tr>
</table>

This table has 6 rows and 4 columns. In several nodes, such as [Get Table Row](vuo-node://vuo.table.get.row) and [Get Table Column](vuo-node://vuo.table.get.column), you can refer to rows and columns by number, counting left-to-right or top-to-bottom starting from 1. For example, the header row of this table is row 1, and the bottom row (Simona Halep) is row 6.

You can also refer to rows and columns by header if there is one. The table above has headers for the columns (but not for the rows). With the [Get Table Column](vuo-node://vuo.table.get.column) node, you could request the row with header *Player* to get a list of all of the players' names.

To switch between these two modes of referring to rows/columns — numbers or headers — change the data type of the Row/Column port. (Right-click on the port and select Integer or Text from the Set Data Type submenu.)

Vuo stores each item of the table as text (even if it's a number or date). With the [Sort Table](vuo-node://vuo.table.sort) node, you have the option to treat the sorted column as a number or date, instead of sorting it alphabetically like text. When you retrieve table items with the [Get Table Row](vuo-node://vuo.table.get.row), [Get Table Column](vuo-node://vuo.table.get.column), or [Get Table Item](vuo-node://vuo.table.get.item) node, the items are output as text. You can convert them to numbers using nodes such as [Convert Text to Integer](vuo-node://vuo.type.text.integer) and [Convert Text List to Real List](vuo-node://vuo.type.list.text.real).
