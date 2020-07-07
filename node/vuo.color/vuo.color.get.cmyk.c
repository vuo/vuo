/**
 * @file
 * vuo.color.get.cmyk node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoColorspace.h"

VuoModuleMetadata({
    "title": "Get CMYK Color Values",
    "keywords": [
        "red", "green", "blue",
        "CMYKA",
        "alpha", "transparent", "opacity",
        "channel", "tone", "chroma",
        "key", // The K in CMYK.
        "color space",
    ],
    "version": "1.1.0",
    "node": {
        "exampleCompositions": [ ],
    },
    "dependencies" : [
        "VuoColorspace",
    ],
});

void nodeEvent(
	VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) color,
	VuoInputData(VuoInteger, {"menuItems":[
		{"value":0, "name":"Linear"},
		{"value":1, "name":"Apple"},
	], "default":0}) colorspace,
	VuoOutputData(VuoReal) cyan,
	VuoOutputData(VuoReal) magenta,
	VuoOutputData(VuoReal) yellow,
	VuoOutputData(VuoReal) black,
	VuoOutputData(VuoReal) alpha)
{
	VuoColorspace_getCMYKA(color, colorspace, cyan, magenta, yellow, black, alpha);
}
