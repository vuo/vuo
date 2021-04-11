/**
 * @file
 * vuo.math.scale node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include <float.h>

VuoModuleMetadata({
    "title": "Scale",
    "keywords": [
        "convert", "unit", "range", "split", "mix", "blend", "lerp", "interpolate",
    ],
    "version": "2.0.0",
    "genericTypes": {
        "VuoGenericType1": {
            "defaultType": "VuoReal",
            "compatibleTypes": [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ],
        },
    },
    "node": {
        "exampleCompositions": [ "vuo-example://vuo.image/MakeStainedGlassImage.vuo", "vuo-example://vuo.image/SimulatePrintedImage.vuo" ],
    },
});

void nodeEvent(
	VuoInputData(VuoGenericType1, {"defaults":{
		"VuoInteger":0,
		"VuoReal":0.,
		"VuoPoint2d":{"x":0.,"y":0.},
		"VuoPoint3d":{"x":0.,"y":0.,"z":0.},
		"VuoPoint4d":{"x":0.,"y":0.,"z":0.,"w":0.}
	}}) value,
	VuoInputData(VuoGenericType1, {"defaults":{
		"VuoInteger":0,
		"VuoReal":0.,
		"VuoPoint2d":{"x":0.,"y":0.},
		"VuoPoint3d":{"x":0.,"y":0.,"z":0.},
		"VuoPoint4d":{"x":0.,"y":0.,"z":0.,"w":0.}
	}}) start,
	VuoInputData(VuoGenericType1, {"defaults":{
		"VuoInteger":10,
		"VuoReal":1.,
		"VuoPoint2d":{"x":1.,"y":1.},
		"VuoPoint3d":{"x":1.,"y":1.,"z":1.},
		"VuoPoint4d":{"x":1.,"y":1.,"z":1.,"w":1.},
	}}) end,
	VuoInputData(VuoGenericType1, {"defaults":{
		"VuoInteger":0,
		"VuoReal":0.,
		"VuoPoint2d":{"x":0.,"y":0.},
		"VuoPoint3d":{"x":0.,"y":0.,"z":0.},
		"VuoPoint4d":{"x":0.,"y":0.,"z":0.,"w":0.},
	}}) scaledStart,
	VuoInputData(VuoGenericType1, {"defaults":{
		"VuoInteger":100,
		"VuoReal":100.,
		"VuoPoint2d":{"x":100.,"y":100.},
		"VuoPoint3d":{"x":100.,"y":100.,"z":100.},
		"VuoPoint4d":{"x":100.,"y":100.,"z":100.,"w":100.},
	}}) scaledEnd,
	VuoInputData(VuoBoolean, {"default":false, "name":"Limit to Range"}) limitToRange,
	VuoOutputData(VuoGenericType1) scaledValue)
{
	VuoGenericType1 clampedValue = limitToRange ? VuoGenericType1_clampn(value, start, end) : value;
	VuoGenericType1 range = VuoGenericType1_subtract(end, start);
	range = VuoGenericType1_makeNonzero(range);
	VuoGenericType1 scaledRange = VuoGenericType1_subtract(scaledEnd, scaledStart);
	*scaledValue = VuoGenericType1_add( VuoGenericType1_divide( VuoGenericType1_scale( VuoGenericType1_subtract(clampedValue, start), scaledRange), range), scaledStart);
}
