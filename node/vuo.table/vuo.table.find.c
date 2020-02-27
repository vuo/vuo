/**
 * @file
 * vuo.table.find node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
	"title" : "Find Table Row",
	"keywords" : [
		"pick", "select", "choose",
		"search", "look up", "retrieve",
		"cell", "item",
		"first",
	],
	"version" : "1.0.0",
	"genericTypes" : {
		"VuoGenericType1" : {
			"compatibleTypes" : [ "VuoInteger", "VuoText" ],
			"defaultType" : "VuoInteger"
		}
	},
	"node" : {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent(
	VuoInputData(VuoTable) table,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":1, "VuoText":""}}) column,
	VuoInputData(VuoText) value,
	VuoInputData(VuoTextComparison, {"default":{"type":"equals","isCaseSensitive":false}}) valueComparison,
	VuoInputData(VuoBoolean, {"default":true}) includeHeader,
	VuoOutputData(VuoList_VuoText) foundRowValues
)
{
	*foundRowValues = VuoTable_findFirstMatchingRow_VuoGenericType1(table, column, value, valueComparison, includeHeader);
}
