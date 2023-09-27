/**
 * @file
 * vuo.math.areEqual node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Are Equal (Math)",
	"keywords": [
		"==", "same", "identical", "equivalent", "match", "compare", "conditional",
		"approximate", "range", "roughly", "~", "≈",
	],
	"version": "2.1.0",
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
	VuoInputData(VuoGenericType1, {
		"defaults":{
			"VuoInteger":0,
			"VuoReal":0.00001,
			"VuoPoint2d":[0.00001,0.00001],
			"VuoPoint3d":[0.00001,0.00001,0.00001],
			"VuoPoint4d":[0.00001,0.00001,0.00001,0.00001],
		},
		"suggestedMin":{
			"VuoInteger":0,
			"VuoReal":0,
			"VuoPoint2d":[0,0],
			"VuoPoint3d":[0,0,0],
			"VuoPoint4d":[0,0,0,0],
		}}) tolerance,
	VuoOutputData(VuoBoolean) equal)
{
	*equal = VuoGenericType1_areEqualListWithinTolerance(values, tolerance);
}
