/**
 * @file
 * VuoTable implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include <vector>
using namespace std;

extern "C"
{
#include "type.h"
#include "VuoTable.h"
#include "VuoTime.h"

#include <csv.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Table",
					  "description" : "Information structured in rows and columns.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoTable",
						  "VuoTableFormat",
						  "VuoTextSort",
						  "VuoSortOrder",
						  "VuoText",
						  "VuoList_VuoText",
						  "VuoTime",
						  "csv"
					  ]
				  });
#endif
/// @}
}

/**
 * Deallocates a `vector< vector<VuoText> > *`.
 */
static void deleteData(void *data)
{
	delete (vector< vector<VuoText> > *)data;
}


/**
 * Decodes the JSON object @a js to create a new value.
 */
VuoTable VuoTable_makeFromJson(struct json_object *js)
{
	json_object *o;
	if (json_object_object_get_ex(js, "pointer", &o))
	{
		VuoTable table = { NULL, 0, 0 };

		table.data = (void *)json_object_get_int64(o);
		if (! table.data)
		{
			table.data = new vector< vector<VuoText> >;
			VuoRegister(table.data, deleteData);
		}

		if (json_object_object_get_ex(js, "rowCount", &o))
			table.rowCount = (size_t)json_object_get_int64(o);

		if (json_object_object_get_ex(js, "columnCount", &o))
			table.columnCount = (size_t)json_object_get_int64(o);

		return table;
	}
	else if (json_object_object_get_ex(js, "csv", &o))
	{
		const char *tableAsCsv = json_object_get_string(o);

		return VuoTable_makeFromText(tableAsCsv, VuoTableFormat_Csv);
	}

	return VuoTable_makeEmpty();
}

/**
 * Encodes @a value as a JSON object that can only be decoded in the same process.
 */
struct json_object * VuoTable_getJson(const VuoTable value)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "pointer", json_object_new_int64((int64_t)value.data));
	json_object_object_add(js, "rowCount", json_object_new_int64((int64_t)value.rowCount));
	json_object_object_add(js, "columnCount", json_object_new_int64((int64_t)value.columnCount));

	return js;
}

/**
 * Encodes @a value as a JSON object that can be decoded in a separate process.
 */
struct json_object * VuoTable_getInterprocessJson(const VuoTable value)
{
	json_object *js = json_object_new_object();

	VuoText tableAsCsv = VuoTable_serialize(value, VuoTableFormat_Csv);
	json_object_object_add(js, "csv", json_object_new_string(tableAsCsv));
	VuoRetain(tableAsCsv);
	VuoRelease(tableAsCsv);

	return js;
}

/**
 * Returns a human-readable description of @a value.
 */
char * VuoTable_getSummary(const VuoTable value)
{
	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)value.data;

	const size_t maxTableRows = 4;     // not counting ellipsis
	const size_t maxTableColumns = 4;  //

	ostringstream oss;
	oss << "<table>";

	for (size_t i = 0; i < value.rowCount && i < maxTableRows; ++i)
	{
		oss << "<tr>";

		for (size_t j = 0; j < value.columnCount && j < maxTableColumns; ++j)
		{
			const char *dataValue = "";
			if (i < (*data).size() && j < (*data)[i].size() && (*data)[i][j])
				dataValue = (*data)[i][j];
			oss << "<td>" << dataValue << "</td>";
		}

		if (value.columnCount > maxTableColumns)
			oss << "<td>…</td>";

		oss << "</tr>";
	}

	if (value.rowCount > maxTableRows)
	{
		size_t ellipsisColumn = (value.columnCount > maxTableColumns ? 1 : 0);
		oss << "<tr><td colspan=\"" << MIN(value.columnCount, maxTableColumns + ellipsisColumn) << "\">…</td></tr>";
	}

	oss << "</table>";

	return VuoText_format("%lu row%s, %lu column%s<br>%s",
						  value.rowCount, value.rowCount == 1 ? "" : "s",
						  value.columnCount, value.columnCount == 1 ? "" : "s",
						  oss.str().c_str());
}


/**
 * Returns a table with nothing in it.
 */
VuoTable VuoTable_makeEmpty(void)
{
	VuoTable table = { new vector< vector<VuoText> >, 0, 0 };
	VuoRegister(table.data, deleteData);
	return table;
}


/**
 * Helper for VuoTable_makeFromText(). Table data and parsing state tracked while parsing a CSV/TSV file.
 */
class ParserContext
{
public:
	///@{
	/**
	 * Parsing state.
	 */
	vector< vector<VuoText> > *tableData;
	bool atFirstColumn;
	///@}

	bool taken;  ///< If true, this class has surrendered ownership of data.

	ParserContext(void)
	{
		tableData = new vector< vector<VuoText> >;
		atFirstColumn = true;
		taken = false;
	}

	~ParserContext(void)
	{
		if (! taken)
			delete tableData;
	}
};

/**
 * Helper for VuoTable_makeFromText(). Callback for when the parser has gotten a single item of table data.
 */
static void parserGotItem(void *item, size_t numBytes, void *userData)
{
	VuoText itemText = VuoText_makeFromData((unsigned char *)item, numBytes);
	ParserContext *ctx = (ParserContext *)userData;

	if (ctx->atFirstColumn)
	{
		ctx->tableData->push_back( vector<VuoText>() );
		ctx->atFirstColumn = false;
	}

	ctx->tableData->back().push_back(itemText);
}

/**
 * Helper for VuoTable_makeFromText(). Callback for when the parser has reached the end of a line.
 */
static void parserGotLine(int lineEnd, void *userData)
{
	ParserContext *ctx = (ParserContext *)userData;

	ctx->atFirstColumn = true;
}

/**
 * Returns a table parsed from @a text, a CSV- or TSV-formatted string.
 */
VuoTable VuoTable_makeFromText(VuoText text, VuoTableFormat format)
{
	struct csv_parser parser;
	unsigned char options = CSV_APPEND_NULL;

	int ret = csv_init(&parser, options);
	if (ret)
	{
		VUserLog("Error: Couldn't initialize CSV/TSV parser.");
		return VuoTable_makeEmpty();
	}

	if (format == VuoTableFormat_Tsv)
		csv_set_delim(&parser, CSV_TAB);

	ParserContext ctx;

	size_t numBytes = VuoText_byteCount(text);
	size_t numBytesParsed = csv_parse(&parser, text, numBytes, parserGotItem, parserGotLine, &ctx);
	if (numBytesParsed != numBytes)
	{
		VUserLog("Error: Couldn't parse CSV/TSV text: %s", csv_strerror(csv_error(&parser)));
		csv_free(&parser);
		return VuoTable_makeEmpty();
	}

	ret = csv_fini(&parser, parserGotItem, parserGotLine, &ctx);
	if (ret)
	{
		VUserLog("Error: Couldn't parse CSV/TSV text (or finalize parser): %s", csv_strerror(csv_error(&parser)));
		csv_free(&parser);
		return VuoTable_makeEmpty();
	}

	csv_free(&parser);
	ctx.taken = true;

	size_t rowCount = ctx.tableData->size();
	size_t columnCount = 0;
	for (vector< vector<VuoText> >::iterator i = ctx.tableData->begin(); i != ctx.tableData->end(); ++i)
		columnCount = MAX((*i).size(), columnCount);

	VuoTable table = { (void *)ctx.tableData, rowCount, columnCount };
	VuoRegister(table.data, deleteData);
	return table;
}


/**
 * Returns a CSV- or TSV-formatted string representation of a table.
 */
VuoText VuoTable_serialize(VuoTable table, VuoTableFormat format)
{
	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;

	// Calculate the sizes of the strings to be written.

	vector< vector<size_t> > srcBytesForData( (*data).size() );
	vector< vector<size_t> > dstBytesForData( (*data).size() );
	size_t dstBytesTotal = 0;

	for (size_t i = 0; i < table.rowCount; ++i)
	{
		if (i < (*data).size())
		{
			srcBytesForData[i].resize( (*data)[i].size() );
			dstBytesForData[i].resize( (*data)[i].size() );
		}

		for (size_t j = 0; j < table.columnCount; ++j)
		{
			if (i < (*data).size() && j < (*data)[i].size())
			{
				srcBytesForData[i][j] = VuoText_byteCount( (*data)[i][j] );
				dstBytesForData[i][j] = csv_write(NULL, 0, (*data)[i][j], srcBytesForData[i][j]);
				dstBytesTotal += dstBytesForData[i][j];
			}

			++dstBytesTotal;  // delimiter or newline
		}
	}

	// Write the strings.

	char delimiter = (format == VuoTableFormat_Tsv ? CSV_TAB : CSV_COMMA);
	char newline = '\n';

	char *dst = (char *)malloc(dstBytesTotal);
	char *dstPtr = dst;

	for (size_t i = 0; i < table.rowCount; ++i)
	{
		for (size_t j = 0; j < table.columnCount; ++j)
		{
			if (i < (*data).size() && j < (*data)[i].size())
			{
				size_t numBytesWritten = csv_write(dstPtr, dstBytesForData[i][j], (*data)[i][j], srcBytesForData[i][j]);
				dstPtr += numBytesWritten;
			}

			*dstPtr = (j+1 < table.columnCount ? delimiter : newline);
			++dstPtr;
		}
	}

	if (dstPtr)
	{
		--dstPtr;
		*dstPtr = 0;
	}

	VuoText text = VuoText_make(dst);
	free(dst);
	return text;
}


/**
 * Returns the index (indexed from 1) of the first column/row whose header (value in first row/column)
 * contains @a header (case-insensitive comparison).
 */
static VuoInteger getIndexForHeader(VuoTable table, VuoText header, bool isColumnHeader)
{
	if (VuoText_isEmpty(header))
		return 0;

	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;

	VuoTextComparison containsCaseInsensitive = { VuoTextComparison_Contains, false };

	if (isColumnHeader)
	{
		for (size_t i = 0; i < table.columnCount; ++i)
			if (! (*data).empty() && i < (*data)[0].size() && VuoText_compare((*data)[0][i], containsCaseInsensitive, header))
				return i+1;
	}
	else
	{
		for (size_t i = 0; i < table.rowCount; ++i)
			if (i < (*data).size() && ! (*data)[i].empty() && VuoText_compare((*data)[i][0], containsCaseInsensitive, header))
				return i+1;
	}

	return 0;
}

/**
 * Returns @a index if it's in bounds, or the closest in-bounds index if not (indexed from 1).
 */
static VuoInteger getClampedIndex(VuoTable table, VuoInteger index, bool isColumnIndex)
{
	VuoInteger maxIndex = (isColumnIndex ? table.columnCount : table.rowCount);  // converts from size_t to VuoInteger
	return MAX(1, MIN(maxIndex, index));
}


/**
 * Helper for `VuoTable_sort_VuoInteger()`.
 */
struct ColumnSort
{
	///@{
	/**
	 * Sort parameters used by `operator()`.
	 */
	VuoTextSort sortType;
	VuoSortOrder sortOrder;
	///@}

	/**
	 * Returns true if the text in @a first comes before the text in @a second in sorted order.
	 */
	bool operator() (pair<size_t, VuoText> first, pair<size_t, VuoText> second)
	{
		if (sortOrder == VuoSortOrder_Descending)
			swap(first, second);

		if (sortType == VuoTextSort_TextCaseSensitive)
			return VuoText_isLessThan(first.second, second.second);
		else if (sortType == VuoTextSort_Number)
			return VuoText_isLessThanNumeric(first.second, second.second);
		else if (sortType == VuoTextSort_Date)
		{
			VuoTime firstTime = VuoTime_makeFromUnknownFormat(first.second);
			VuoTime secondTime = VuoTime_makeFromUnknownFormat(second.second);
			return VuoTime_isLessThan(firstTime, secondTime);
		}
		else
			return VuoText_isLessThanCaseInsensitive(first.second, second.second);
	}
};

/**
 * Sorts the table's rows based on the values in the given column.
 *
 * @a columnIndex is clamped to a valid index (between 1 and `columnCount`).
 *
 * If @a firstRowIsHeader is true, the first row is left in place and the remaining rows are sorted.
 *
 * If multiple items in the selected column have the same value, this function preserves their ordering
 * (stable sort).
 */
VuoTable VuoTable_sort_VuoInteger(VuoTable table, VuoInteger columnIndex, VuoTextSort sortType, VuoSortOrder sortOrder, bool firstRowIsHeader)
{
	columnIndex = getClampedIndex(table, columnIndex, true);  // indexed from 1
	size_t firstDataRow = (firstRowIsHeader ? 1 : 0);  // indexed from 0

	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;

	vector< pair<size_t, VuoText> > keyColumn;
	for (size_t i = firstDataRow; i < table.rowCount; ++i)
	{
		if (i < (*data).size() && columnIndex-1 < (*data)[i].size())
			keyColumn.push_back( make_pair(i, (*data)[i][columnIndex-1]) );
		else
			keyColumn.push_back( make_pair(i, "") );
	}

	ColumnSort columnSort;
	columnSort.sortType = sortType;
	columnSort.sortOrder = sortOrder;
	stable_sort(keyColumn.begin(), keyColumn.end(), columnSort);

	if (firstRowIsHeader)
		keyColumn.insert(keyColumn.begin(), make_pair(0, "") );

	vector< vector<VuoText> > *sortedData = new vector< vector<VuoText> >(table.rowCount);
	for (size_t i = 0; i < table.rowCount; ++i)
		(*sortedData)[i].resize(table.columnCount);

	for (size_t i = 0; i < table.rowCount; ++i)
		for (size_t j = 0; j < table.columnCount; ++j)
			if (i < (*data).size() && j < (*data)[i].size())
				(*sortedData)[i][j] = (*data)[ keyColumn[i].first ][j];

	VuoTable sortedTable = { sortedData, table.rowCount, table.columnCount };
	VuoRegister(sortedData, deleteData);
	return sortedTable;
}

/**
 * Sorts the table's rows based on the values in the given column.
 *
 * The first column whose header (value in first row) contains @a columnHeader (case-insensitive comparison) is selected.
 * If no such column exists, the original (unsorted) table is returned.
 */
VuoTable VuoTable_sort_VuoText(VuoTable table, VuoText columnHeader, VuoTextSort sortType, VuoSortOrder sortOrder, bool firstRowIsHeader)
{
	VuoInteger columnIndex = getIndexForHeader(table, columnHeader, true);
	if (columnIndex == 0)
		return table;

	return VuoTable_sort_VuoInteger(table, columnIndex, sortType, sortOrder, firstRowIsHeader);
}


/**
 * Diagonally flips the table.
 */
VuoTable VuoTable_transpose(VuoTable table)
{
	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;

	vector< vector<VuoText> > *transposedData = new vector< vector<VuoText> >(table.columnCount);
	for (size_t i = 0; i < table.columnCount; ++i)
		(*transposedData)[i].resize(table.rowCount);

	for (size_t i = 0; i < table.rowCount; ++i)
		for (size_t j = 0; j < table.columnCount; ++j)
			if (i < (*data).size() && j < (*data)[i].size())
				(*transposedData)[j][i] = (*data)[i][j];

	VuoTable transposedTable = { transposedData, table.columnCount, table.rowCount };
	VuoRegister(transposedData, deleteData);
	return transposedTable;
}


/**
 * Returns the data values in the row at @a rowIndex (indexed from 1).
 * Empty cells are returned as NULL.
 */
VuoList_VuoText VuoTable_getRow_VuoInteger(VuoTable table, VuoInteger rowIndex, bool includeHeader)
{
	if (table.rowCount == 0)
		return VuoListCreate_VuoText();

	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;

	rowIndex = getClampedIndex(table, rowIndex, false);

	size_t startIndex = (includeHeader ? 0 : 1);
	size_t count = table.columnCount - startIndex;

	VuoList_VuoText row = VuoListCreateWithCount_VuoText(count, NULL);
	for (size_t i = startIndex; i < table.columnCount; ++i)
		if (rowIndex-1 < (*data).size() && i < (*data)[rowIndex-1].size())
			VuoListSetValue_VuoText(row, (*data)[rowIndex-1][i], i+1-startIndex, false);

	return row;
}

/**
 * Returns the data values in the row whose header (value in first column) matches @a rowHeader.
 * Empty cells are returned as NULL.
 */
VuoList_VuoText VuoTable_getRow_VuoText(VuoTable table, VuoText rowHeader, bool includeHeader)
{
	VuoInteger rowIndex = getIndexForHeader(table, rowHeader, false);
	if (rowIndex == 0)
		return VuoListCreate_VuoText();

	return VuoTable_getRow_VuoInteger(table, rowIndex, includeHeader);
}

/**
 * Returns the data values in the column at @a columnIndex (indexed from 1).
 * Empty cells are returned as NULL.
 */
VuoList_VuoText VuoTable_getColumn_VuoInteger(VuoTable table, VuoInteger columnIndex, bool includeHeader)
{
	if (table.rowCount == 0)
		return VuoListCreate_VuoText();

	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;

	columnIndex = getClampedIndex(table, columnIndex, true);

	size_t startIndex = (includeHeader ? 0 : 1);
	size_t count = table.rowCount - startIndex;

	VuoList_VuoText column = VuoListCreateWithCount_VuoText(count, NULL);
	for (size_t i = startIndex; i < table.rowCount; ++i)
		if (i < (*data).size() && columnIndex-1 < (*data)[i].size())
			VuoListSetValue_VuoText(column, (*data)[i][columnIndex-1], i+1-startIndex, false);

	return column;
}

/**
 * Returns the data values in the column whose header (value in first row) matches @a columnHeader.
 * Empty cells are returned as NULL.
 */
VuoList_VuoText VuoTable_getColumn_VuoText(VuoTable table, VuoText columnHeader, bool includeHeader)
{
	VuoInteger columnIndex = getIndexForHeader(table, columnHeader, true);
	if (columnIndex == 0)
		return VuoListCreate_VuoText();

	return VuoTable_getColumn_VuoInteger(table, columnIndex, includeHeader);
}

/**
 * Returns the data value at row @a rowIndex and column @a columnIndex.
 */
VuoText VuoTable_getItem_VuoInteger_VuoInteger(VuoTable table, VuoInteger rowIndex, VuoInteger columnIndex)
{
	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;
	if ((*data).empty())
		return NULL;

	rowIndex = getClampedIndex(table, rowIndex, false);
	columnIndex = getClampedIndex(table, columnIndex, true);

	return (*data)[rowIndex-1][columnIndex-1];
}

/**
 * Returns the data value at row @a rowIndex and column with header @a columnHeader.
 */
VuoText VuoTable_getItem_VuoInteger_VuoText(VuoTable table, VuoInteger rowIndex, VuoText columnHeader)
{
	VuoInteger columnIndex = getIndexForHeader(table, columnHeader, true);
	if (columnIndex == 0)
		return NULL;

	return VuoTable_getItem_VuoInteger_VuoInteger(table, rowIndex, columnIndex);
}

/**
 * Returns the data value at row with header @a rowHeader and column @a columnIndex.
 */
VuoText VuoTable_getItem_VuoText_VuoInteger(VuoTable table, VuoText rowHeader, VuoInteger columnIndex)
{
	VuoInteger rowIndex = getIndexForHeader(table, rowHeader, false);
	if (rowIndex == 0)
		return NULL;

	return VuoTable_getItem_VuoInteger_VuoInteger(table, rowIndex, columnIndex);
}

/**
 * Returns the data value at row with header @a rowHeader and column with header @a columnHeader.
 */
VuoText VuoTable_getItem_VuoText_VuoText(VuoTable table, VuoText rowHeader, VuoText columnHeader)
{
	VuoInteger rowIndex = getIndexForHeader(table, rowHeader, false);
	VuoInteger columnIndex = getIndexForHeader(table, columnHeader, true);
	if (rowIndex == 0 || columnIndex == 0)
		return NULL;

	return VuoTable_getItem_VuoInteger_VuoInteger(table, rowIndex, columnIndex);
}

/**
 * Returns a table in which @a values has been added as a new row, either before the first or after the last row.
 */
VuoTable VuoTable_addRow(VuoTable table, VuoListPosition position, VuoList_VuoText values)
{
	vector< vector<VuoText> > *origData = (vector< vector<VuoText> > *)table.data;
	vector< vector<VuoText> > *data = new vector< vector<VuoText> >(*origData);

	vector< vector<VuoText> >::iterator rowIter;
	size_t rowIndex;
	if (position == VuoListPosition_Beginning)
	{
		rowIter = (*data).begin();
		rowIndex = 0;
	}
	else
	{
		rowIter = (*data).end();
		rowIndex = table.rowCount;
	}

	unsigned long count = VuoListGetCount_VuoText(values);
	VuoText *rowArray = VuoListGetData_VuoText(values);

	(*data).insert(rowIter, vector<VuoText>(count));
	for (unsigned long i = 0; i < count; ++i)
		(*data)[rowIndex][i] = rowArray[i];

	VuoTable newTable = { data, table.rowCount + 1, MAX(table.columnCount, count) };
	VuoRegister(newTable.data, deleteData);
	return newTable;
}

/**
 * Returns a table in which @a values has been added as a new column, either before the first or after the last column.
 */
VuoTable VuoTable_addColumn(VuoTable table, VuoListPosition position, VuoList_VuoText values)
{
	vector< vector<VuoText> > *origData = (vector< vector<VuoText> > *)table.data;
	vector< vector<VuoText> > *data = new vector< vector<VuoText> >(*origData);

	unsigned long count = VuoListGetCount_VuoText(values);
	VuoText *columnArray = VuoListGetData_VuoText(values);

	if ((*data).size() < count)
		(*data).resize(count);

	for (size_t i = 0; i < count; ++i)
	{
		if (position == VuoListPosition_End && (*data)[i].size() < table.columnCount)
			(*data)[i].resize(table.columnCount);

		vector<VuoText>::iterator columnIter = (position == VuoListPosition_Beginning ? (*data)[i].begin() : (*data)[i].end());
		(*data)[i].insert(columnIter, columnArray[i]);
	}

	VuoTable newTable = { data, MAX(table.rowCount, count), table.columnCount + 1 };
	VuoRegister(newTable.data, deleteData);
	return newTable;
}

/**
 * Returns a table in which @a newValues has replaced the row at @a rowIndex.
 */
VuoTable VuoTable_changeRow_VuoInteger(VuoTable table, VuoInteger rowIndex, VuoList_VuoText newValues, bool preserveHeader)
{
	if (table.rowCount == 0)
		return table;

	rowIndex = getClampedIndex(table, rowIndex, false);

	vector< vector<VuoText> > *origData = (vector< vector<VuoText> > *)table.data;
	vector< vector<VuoText> > *data = new vector< vector<VuoText> >(*origData);

	unsigned long newValuesCount = VuoListGetCount_VuoText(newValues);
	VuoText *rowArray = VuoListGetData_VuoText(newValues);

	size_t firstDataIndex = (preserveHeader ? 1 : 0);

	(*data)[rowIndex-1].resize(newValuesCount+firstDataIndex);

	if (preserveHeader && ! (*origData)[rowIndex-1].empty())
		(*data)[rowIndex-1][0] = (*origData)[rowIndex-1][0];

	for (size_t i = 0; i < newValuesCount; ++i)
		(*data)[rowIndex-1][i+firstDataIndex] = rowArray[i];

	VuoTable newTable = { data, table.rowCount, MAX(table.columnCount, newValuesCount+firstDataIndex) };
	VuoRegister(newTable.data, deleteData);
	return newTable;
}

/**
 * Returns a table in which @a newValues has replaced the row with header @a rowHeader.
 */
VuoTable VuoTable_changeRow_VuoText(VuoTable table, VuoText rowHeader, VuoList_VuoText newValues, bool preserveHeader)
{
	VuoInteger rowIndex = getIndexForHeader(table, rowHeader, false);
	if (rowIndex == 0)
		return table;

	return VuoTable_changeRow_VuoInteger(table, rowIndex, newValues, preserveHeader);
}

/**
 * Returns a table in which @a newValues has replaced the column at @a columnIndex.
 */
VuoTable VuoTable_changeColumn_VuoInteger(VuoTable table, VuoInteger columnIndex, VuoList_VuoText newValues, bool preserveHeader)
{
	if (table.columnCount == 0)
		return table;

	columnIndex = getClampedIndex(table, columnIndex, true);

	vector< vector<VuoText> > *origData = (vector< vector<VuoText> > *)table.data;
	vector< vector<VuoText> > *data = new vector< vector<VuoText> >(*origData);

	unsigned long newValuesCount = VuoListGetCount_VuoText(newValues);
	VuoText *columnArray = VuoListGetData_VuoText(newValues);

	size_t firstDataIndex = (preserveHeader ? 1 : 0);

	if ((*data).size() < newValuesCount+firstDataIndex)
		(*data).resize(newValuesCount+firstDataIndex);

	for (size_t i = 0; i < newValuesCount+firstDataIndex; ++i)
		if ((*data)[i].size() < columnIndex)
			(*data)[i].resize(columnIndex);

	if (preserveHeader && (*origData)[0].size() > columnIndex-1)
		(*data)[0][columnIndex-1] = (*origData)[0][columnIndex-1];

	for (size_t i = 0; i < MAX(table.rowCount-firstDataIndex, newValuesCount); ++i)
		(*data)[i+firstDataIndex][columnIndex-1] = (i < newValuesCount ? columnArray[i] : NULL);

	VuoTable newTable = { data, MAX(table.rowCount, newValuesCount+firstDataIndex), table.columnCount };
	VuoRegister(newTable.data, deleteData);
	return newTable;
}

/**
 * Returns a table in which @a newValues has replaced the column with header @a columnHeader.
 */
VuoTable VuoTable_changeColumn_VuoText(VuoTable table, VuoText columnHeader, VuoList_VuoText newValues, bool preserveHeader)
{
	VuoInteger columnIndex = getIndexForHeader(table, columnHeader, true);
	if (columnIndex == 0)
		return table;

	return VuoTable_changeColumn_VuoInteger(table, columnIndex, newValues, preserveHeader);
}

/**
 * Returns a table in which @a newValue has replaced the value at row @a rowIndex and column @a columnIndex.
 */
VuoTable VuoTable_changeItem_VuoInteger_VuoInteger(VuoTable table, VuoInteger rowIndex, VuoInteger columnIndex, VuoText newValue)
{
	vector< vector<VuoText> > *origData = (vector< vector<VuoText> > *)table.data;
	if ((*origData).empty())
		return table;

	rowIndex = getClampedIndex(table, rowIndex, false);
	columnIndex = getClampedIndex(table, columnIndex, true);

	vector< vector<VuoText> > *data = new vector< vector<VuoText> >(*origData);
	(*data)[rowIndex-1][columnIndex-1] = newValue;

	VuoTable newTable = { data, table.rowCount, table.columnCount };
	VuoRegister(newTable.data, deleteData);
	return newTable;

}

/**
 * Returns a table in which @a newValue has replaced the value at row @a rowIndex and column with header @a columnHeader.
 */
VuoTable VuoTable_changeItem_VuoInteger_VuoText(VuoTable table, VuoInteger rowIndex, VuoText columnHeader, VuoText newValue)
{
	VuoInteger columnIndex = getIndexForHeader(table, columnHeader, true);
	if (columnIndex == 0)
		return table;

	return VuoTable_changeItem_VuoInteger_VuoInteger(table, rowIndex, columnIndex, newValue);
}

/**
 * Returns a table in which @a newValue has replaced the value at row with header @a rowHeader and column @a columnIndex.
 */
VuoTable VuoTable_changeItem_VuoText_VuoInteger(VuoTable table, VuoText rowHeader, VuoInteger columnIndex, VuoText newValue)
{
	VuoInteger rowIndex = getIndexForHeader(table, rowHeader, false);
	if (rowIndex == 0)
		return table;

	return VuoTable_changeItem_VuoInteger_VuoInteger(table, rowIndex, columnIndex, newValue);
}

/**
 * Returns a table in which @a newValue has replaced the value at row with header @a rowHeader and column with header @a columnHeader.
 */
VuoTable VuoTable_changeItem_VuoText_VuoText(VuoTable table, VuoText rowHeader, VuoText columnHeader, VuoText newValue)
{
	VuoInteger rowIndex = getIndexForHeader(table, rowHeader, false);
	VuoInteger columnIndex = getIndexForHeader(table, columnHeader, true);
	if (rowIndex == 0 || columnIndex == 0)
		return table;

	return VuoTable_changeItem_VuoInteger_VuoInteger(table, rowIndex, columnIndex, newValue);
}

/**
 * Returns a table in which the first or last row has been removed.
 */
VuoTable VuoTable_removeRow(VuoTable table, VuoListPosition position)
{
	if (table.rowCount == 0)
		return table;

	vector< vector<VuoText> > *origData = (vector< vector<VuoText> > *)table.data;
	vector< vector<VuoText> > *data = new vector< vector<VuoText> >(*origData);

	vector< vector<VuoText> >::iterator rowIter = (position == VuoListPosition_Beginning ? (*data).begin() : (*data).end() - 1);
	(*data).erase(rowIter);

	VuoTable newTable = { data, table.rowCount - 1, table.columnCount };
	VuoRegister(newTable.data, deleteData);
	return newTable;
}

/**
 * Returns a table in which the first or last column has been removed.
 */
VuoTable VuoTable_removeColumn(VuoTable table, VuoListPosition position)
{
	if (table.columnCount == 0)
		return table;

	vector< vector<VuoText> > *origData = (vector< vector<VuoText> > *)table.data;
	vector< vector<VuoText> > *data = new vector< vector<VuoText> >(*origData);

	for (size_t i = 0; i < table.rowCount; ++i)
	{
		if (position == VuoListPosition_End && (*data)[i].size() < table.columnCount)
			continue;

		vector<VuoText>::iterator columnIter = (position == VuoListPosition_Beginning ? (*data)[i].begin() : (*data)[i].end() - 1);
		(*data)[i].erase(columnIter);
	}

	VuoTable newTable = { data, table.rowCount, table.columnCount - 1 };
	VuoRegister(newTable.data, deleteData);
	return newTable;
}


/**
 * Replaces the auto-generated function in order to retain the items within vectors.
 */
void VuoTable_retain(VuoTable table)
{
	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;
	for (vector< vector<VuoText> >::iterator i = data->begin(); i != data->end(); ++i)
		for (vector<VuoText>::iterator j = i->begin(); j != i->end(); ++j)
			VuoRetain(*j);

	VuoRetain(table.data);
}

/**
 * Replaces the auto-generated function in order to release the items within vectors.
 */
void VuoTable_release(VuoTable table)
{
	vector< vector<VuoText> > *data = (vector< vector<VuoText> > *)table.data;
	for (vector< vector<VuoText> >::iterator i = data->begin(); i != data->end(); ++i)
		for (vector<VuoText>::iterator j = i->begin(); j != i->end(); ++j)
			VuoRelease(*j);

	VuoRelease(table.data);
}
