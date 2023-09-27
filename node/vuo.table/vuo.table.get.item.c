/**
 * @file
 * vuo.table.get.item node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Get Table Item",
					  "keywords" : [
						  "pick", "select", "choose", "index",
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
						  "exampleCompositions" : [ "ChangeTableDataToMetric.vuo", "GraphColdestTemperatures.vuo", "DisplayPetAdoptionsTable.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTable) table,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":1, "VuoText":""}}) row,
		VuoInputData(VuoGenericType2, {"defaults":{"VuoInteger":1, "VuoText":""}}) column,
		VuoOutputData(VuoText) value
)
{
	*value = VuoTable_getItem_VuoGenericType1_VuoGenericType2(table, row, column);
}
