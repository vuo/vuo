/**
 * @file
 * vuo.table.add.column node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Add Table Column",
					  "keywords" : [
						  "push", "append", "prepend", "insert", "combine",
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
	*modifiedTable = VuoTable_addColumn(table, position, values);
}
