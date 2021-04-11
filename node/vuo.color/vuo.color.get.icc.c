/**
 * @file
 * vuo.color.get.icc node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoColorspace.h"

VuoModuleMetadata({
	"title": "Get ICC Color Values",
	"keywords":[
		"red", "green", "blue", "RGBA",
		"sRGB", "Adobe RGB", "Generic RGB",
		"cyan", "magenta", "yellow", "key", "black", "CMYKA",
		"CIE 1931 XYZ", "CIEXYZ",
		"CIELAB D50", "L*a*b*", "Lab",
		"alpha", "transparent", "opacity",
		"channel", "tone", "chroma",
		"lightness",
		"color space", "color profile",
	],
	"version": "1.0.0",
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": "VuoInteger",
			"compatibleTypes": [ "VuoInteger", "VuoData" ],
		},
	},
	"node": {
		"exampleCompositions": [ ],
	},
	"dependencies" : [
		"VuoColorspace",
	],
});

void nodeEvent(
	VuoInputData(VuoColor) color,
	VuoInputData(VuoGenericType1, {"menuItems":[
		{"value":0, "name":"sRGB"},
		{"value":1, "name":"Adobe RGB (1998)"},
		{"value":2, "name":"Generic RGB"},
		{"value":3, "name":"Generic CMYK"},
		{"value":4, "name":"Generic L*a*b*"},
		{"value":5, "name":"Generic XYZ"},
	], "default":3}) colorspace,
	VuoOutputData(VuoList_VuoReal) components)
{
	*components = VuoColorspace_getICC_VuoGenericType1(colorspace, color);
}
