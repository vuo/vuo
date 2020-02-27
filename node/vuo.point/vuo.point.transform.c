/**
 * @file
 * vuo.point.transform node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Transform Point",
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
	VuoInputData(VuoGenericType2, {"default":{"x":0,"y":0,"z":0}}) point,
	VuoOutputData(VuoGenericType2) transformedPoint)
{
	*transformedPoint = VuoGenericType1_transform_VuoGenericType2(transform, point);
}
