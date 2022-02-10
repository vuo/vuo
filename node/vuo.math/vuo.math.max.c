/**
 * @file
 * vuo.math.max node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Find Maximum",
	"keywords": [
		"greatest", "large", "big", "high", "more", "most", ">", "top", "upper", "peak", "limit", "bound", "range",
	],
	"version": "2.2.0",
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": "VuoReal",
			"compatibleTypes": [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ],
		},
	},
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoGenericType1) values,
	VuoOutputData(VuoGenericType1, {"name":"Maximum"}) max,
	VuoOutputData(VuoInteger) position)
{
	*max = VuoGenericType1_maxList(values, position);
}
