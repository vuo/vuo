/**
 * @file
 * vuo.table.add.row node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Add Table Row",
					  "keywords" : [
						  "push", "append", "prepend", "insert", "combine",
						  "cell", "item"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "RecordMouseClicksToCsvFile.vuo", "PerformCoinFlipExperiment.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTable) table,
		VuoInputData(VuoListPosition, {"default":"end"}) position,
		VuoInputData(VuoList_VuoText) values,
		VuoOutputData(VuoTable) modifiedTable
)
{
	*modifiedTable = VuoTable_addRow(table, position, values);
}
