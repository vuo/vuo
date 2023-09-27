/**
 * @file
 * vuo.table.take.column node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Take Column from Table",
					  "keywords" : [
						  "pop", "remove", "delete", "truncate", "cut", "extract",
						  "cell", "item"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTable) table,
		VuoInputData(VuoListPosition, {"default":"beginning"}) position,
		VuoInputData(VuoBoolean, {"default":true}) includeHeader,
		VuoOutputData(VuoList_VuoText) removedColumn,
		VuoOutputData(VuoTable) modifiedTable
)
{
	VuoInteger columnIndex = (position == VuoListPosition_Beginning ? 1 : table.columnCount);
	*removedColumn = VuoTable_getColumn_VuoInteger(table, columnIndex, includeHeader);
	*modifiedTable = VuoTable_removeColumn(table, position);
}
