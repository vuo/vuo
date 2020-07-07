/**
 * @file
 * vuo.math.average node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Average",
	"keywords": [
		"mix", "combine", "mean", "midpoint", "middle",
	],
	"version": "2.0.0",
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": "VuoReal",
			"compatibleTypes": [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ],
		},
	},
	"node": {
		"exampleCompositions": [ "FollowMidpoint.vuo" ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoGenericType1) values,
	VuoOutputData(VuoGenericType1) averageValue)
{
	*averageValue = VuoGenericType1_average(values);
}
