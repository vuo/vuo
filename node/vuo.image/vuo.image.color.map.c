/**
 * @file
 * vuo.image.color.map node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Map Image Colors",
					  "keywords" : [ "gradient", "replace", "heatmap", "tint", "tone", "chroma", "recolor", "colorize", "correction", "calibration", "grading", "balance", "filter" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "RecolorMovie.vuo" ]
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
