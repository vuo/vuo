/**
 * @file
 * vuo.table.change.row node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Change Table Row",
					  "keywords" : [
						  "replace", "place", "insert", "substitute", "modify",
						  "cell", "item"
					  ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoInteger", "VuoText" ],
							  "defaultType" : "VuoInteger"
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ "ChangeTableDataToMetric.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTable) table,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":1, "VuoText":""}}) row,
		VuoInputData(VuoList_VuoText) newValues,
		VuoInputData(VuoBoolean, {"default":false}) preserveHeader,
		VuoOutputData(VuoTable) modifiedTable
)
{
	*modifiedTable = VuoTable_changeRow_VuoGenericType1(table, row, newValues, preserveHeader);
}
