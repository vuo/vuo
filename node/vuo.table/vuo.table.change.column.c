/**
 * @file
 * vuo.table.change.column node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Change Table Column",
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
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":1, "VuoText":""}}) column,
		VuoInputData(VuoList_VuoText) newValues,
		VuoInputData(VuoBoolean, {"default":false}) preserveHeader,
		VuoOutputData(VuoTable) modifiedTable
)
{
	*modifiedTable = VuoTable_changeColumn_VuoGenericType1(table, column, newValues, preserveHeader);
}
