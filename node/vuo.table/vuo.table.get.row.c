/**
 * @file
 * vuo.table.get.row node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Get Table Row",
					  "keywords" : [
						  "pick", "select", "choose", "index",
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
						  "exampleCompositions" : [ "ChangeTableDataToMetric.vuo", "GraphColdestTemperatures.vuo", "DisplayPetAdoptionsTable.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTable) table,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":1, "VuoText":""}}) row,
		VuoInputData(VuoBoolean, {"default":true}) includeHeader,
		VuoOutputData(VuoList_VuoText) values
)
{
	*values = VuoTable_getRow_VuoGenericType1(table, row, includeHeader);
}
