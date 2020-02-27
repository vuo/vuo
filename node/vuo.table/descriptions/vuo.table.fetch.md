Loads or downloads a file containing CSV- or TSV-formatted text, and turns the text into a table.

See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.

You can use this node to import data from a spreadsheet. In your spreadsheet application, export the data in [CSV (Comma-Separated Values)](https://en.wikipedia.org/wiki/Comma-separated_values) or [TSV (Tab-Separated Values)](https://en.wikipedia.org/wiki/Tab-separated_values) format.

Each line of the input text (except empty lines) becomes a row of the table. Within a line, each comma or tab marks the end of a column.

Spaces surrounding commas or tabs are ignored (unless enclosed in quotes).

If some rows have more columns than others, the output table has as many columns as the widest row. Columns that weren't present for a row in the input text are populated with empty text in the output table.

If the text is formatted incorrectly, this node outputs an empty table. Check Console.app for details about the error.

This node is a shortcut for [Fetch Data](vuo-node://vuo.data.fetch) -> [Convert Data to Text](vuo-node://vuo.type.data.text) -> [Make Table from Text](vuo-node://vuo.table.make.text).
