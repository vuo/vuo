/**
 * @file
 * vuo.table.format node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Format Table",
					  "keywords" : [
						"row", "column", "grid", "spreadsheet", "structure",
						"csv", "tsv", "comma", "tab", "separated",
						"convert", "serialize", "export", "save", "write"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ChangeTableDataToMetric.vuo", "RecordMouseClicksToCsvFile.vuo", "PerformCoinFlipExperiment.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTable) table,
		VuoInputData(VuoTableFormat, {"default":"csv"}) format,
		VuoOutputData(VuoText) text
)
{
	*text = VuoTable_serialize(table, format);
}
