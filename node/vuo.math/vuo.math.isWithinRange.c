/**
 * @file
 * vuo.math.isWithinRange node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
    "title": "Is within Range",
    "keywords": [ "clamp", "restrict", "wrap", "limit", "bound", "between", "<=", ">=", "~", "compare" ],
    "version": "1.1.0",
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
	VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.0}}) value,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.0}}) minimum,
	VuoInputData(VuoGenericType1, {
		"defaults":{
			"VuoInteger":10,
			"VuoReal":10,
			"VuoPoint2d":[10,10],
			"VuoPoint3d":[10,10,10],
			"VuoPoint4d":[10,10,10,10],
		}}) maximum,
	VuoOutputData(VuoBoolean) withinRange)
{
	*withinRange = VuoGenericType1_isWithinRange(value, minimum, maximum);
}
