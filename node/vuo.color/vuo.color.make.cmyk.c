/**
 * @file
 * vuo.color.make.cmyk node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoColorspace.h"

VuoModuleMetadata({
	"title": "Make CMYK Color",
	"keywords":[
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
	VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) cyan,
	VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) magenta,
	VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) yellow,
	VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) black,
	VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) alpha,
	VuoInputData(VuoInteger, {"menuItems":[
		{"value":0, "name":"Linear"},
		{"value":1, "name":"Apple"},
	], "default":0}) colorspace,
	VuoOutputData(VuoColor) color)
{
	*color = VuoColorspace_makeCMYKAColor(cyan, magenta, yellow, black, alpha, colorspace);
}
