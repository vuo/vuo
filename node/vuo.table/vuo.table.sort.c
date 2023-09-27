/**
 * @file
 * vuo.table.sort node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTable.h"

VuoModuleMetadata({
	"title": "Sort Table",
	"keywords": [
		"organize", "order", "reorder", "alphabetical", "lexicographical",
		"ascending", "descending", "increasing", "decreasing",
		"row", "cell", "item",
	],
	"version": "1.0.0",
	"genericTypes": {
		"VuoGenericType1": {
			"compatibleTypes": [ "VuoInteger", "VuoText" ],
			"defaultType": "VuoInteger",
		},
	},
	"node": {
		"exampleCompositions": [ "GraphColdestTemperatures.vuo" ],
	},
});

extern VuoTable VuoTable_sort_VuoGenericType1(VuoTable, VuoGenericType1, VuoTextSort, VuoSortOrder, bool);

void nodeEvent
(
		VuoInputData(VuoTable) table,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":1, "VuoText":""}}) column,
		VuoInputData(VuoTextSort, {"default":"text"}) sortType,
		VuoInputData(VuoSortOrder, {"default":"ascending"}) sortOrder,
		VuoInputData(VuoBoolean, {"default":false}) firstRowIsHeader,
		VuoOutputData(VuoTable) sortedTable
)
{
	*sortedTable = VuoTable_sort_VuoGenericType1(table, column, sortType, sortOrder, firstRowIsHeader);
}
