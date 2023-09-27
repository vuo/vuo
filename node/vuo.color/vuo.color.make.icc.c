/**
 * @file
 * vuo.color.make.icc node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoColorspace.h"

VuoModuleMetadata({
	"title": "Make ICC Color",
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

extern VuoColor VuoColorspace_makeICCColor_VuoGenericType1(VuoGenericType1 colorspace, VuoList_VuoReal components);

void nodeEvent(
	VuoInputData(VuoGenericType1, {"menuItems":[
		{"value":0, "name":"sRGB"},
		{"value":1, "name":"Adobe RGB (1998)"},
		{"value":2, "name":"Generic RGB"},
		{"value":3, "name":"Generic CMYK"},
		{"value":4, "name":"Generic L*a*b*"},
		{"value":5, "name":"Generic XYZ"},
	], "default":3}) colorspace,
	VuoInputData(VuoList_VuoReal) components,
	VuoOutputData(VuoColor) color)
{
	*color = VuoColorspace_makeICCColor_VuoGenericType1(colorspace, components);
}
