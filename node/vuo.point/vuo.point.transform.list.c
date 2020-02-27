/**
 * @file
 * vuo.point.transform.list node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Transform Points",
	"keywords": [
		"vector",
		"apply", "transformation",
		"rotate", "spin", "yaw", "pitch", "roll",
		"scale", "size", "flip", "invert", "mirror",
		"translate", "move",
	],
	"version": "1.0.0",
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": "VuoTransform",
			"compatibleTypes": [ "VuoTransform", "VuoTransform2d" ]
		},
		"VuoGenericType2": {
			"defaultType": "VuoPoint3d",
			"compatibleTypes": [ "VuoPoint2d", "VuoPoint3d" ]
			},
		},
	"node": {
		"exampleCompositions": [ ]
	}
});

void nodeEvent(
	VuoInputData(VuoGenericType1) transform,
	VuoInputData(VuoList_VuoGenericType2, {"default":{"x":0,"y":0,"z":0}}) points,
	VuoOutputData(VuoList_VuoGenericType2) transformedPoints)
{
	unsigned long count = VuoListGetCount_VuoGenericType2(points);
	if (!count)
	{
		*transformedPoints = NULL;
		return;
	}

	*transformedPoints = VuoListCreateWithCount_VuoGenericType2(count, VuoGenericType2_makeFromJson(NULL));
	VuoGenericType2 *pointData = VuoListGetData_VuoGenericType2(points);
	VuoGenericType2 *transformedPointData = VuoListGetData_VuoGenericType2(*transformedPoints);
	for (unsigned long i = 0; i < count; ++i)
		transformedPointData[i] = VuoGenericType1_transform_VuoGenericType2(transform, pointData[i]);
}
