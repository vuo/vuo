/**
 * @file
 * vuo.table.take.row node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Take Row from Table",
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
		VuoOutputData(VuoList_VuoText) removedRow,
		VuoOutputData(VuoTable) modifiedTable
)
{
	VuoInteger rowIndex = (position == VuoListPosition_Beginning ? 1 : table.rowCount);
	*removedRow = VuoTable_getRow_VuoInteger(table, rowIndex, includeHeader);
	*modifiedTable = VuoTable_removeRow(table, position);
}
