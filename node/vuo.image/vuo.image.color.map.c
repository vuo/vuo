/**
 * @file
 * vuo.image.color.map node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Map Image Brightness to Gradient",
					  "keywords" : [
						  "gradient", "heatmap",
						  "replace", "colorize", "false color", "recolor", "pseudocolor",
						  "luminance", "lightness", "darkness",
						  "tint", "tone", "chroma", "correction", "calibration", "grading", "balance",
						  "filter",
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "RecolorImage.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoList_VuoColor, {"default":[
			{"r":0.21,"g":0.27,"b":0.12,"a":1},
			{"r":0.60,"g":0.60,"b":0.60,"a":1},
			{"r":0.42,"g":0.64,"b":1.00,"a":1}]})
			colors,
		VuoInputData(VuoReal, {"default":1,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) filterOpacity,
		VuoOutputData(VuoImage) mappedImage
)
{
	*mappedImage = VuoImage_mapColors(image, colors, filterOpacity);
}
