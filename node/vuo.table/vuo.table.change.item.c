/**
 * @file
 * vuo.table.change.item node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Change Table Item",
					  "keywords" : [
						  "replace", "place", "insert", "substitute", "modify",
						  "cell"
					  ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoInteger", "VuoText" ],
							  "defaultType" : "VuoInteger"
						  },
						  "VuoGenericType2" : {
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
		VuoInputData(VuoGenericType2, {"defaults":{"VuoInteger":1, "VuoText":""}}) column,
		VuoInputData(VuoText) newValue,
		VuoOutputData(VuoTable) modifiedTable
)
{
	*modifiedTable = VuoTable_changeItem_VuoGenericType1_VuoGenericType2(table, row, column, newValue);
}
