/**
 * @file
 * vuo.image.color.map node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
			{"r":0,"g":1,"b":0,"a":1},	// green
			{"r":1,"g":0,"b":0,"a":1},	// red
			{"r":1,"g":1,"b":0,"a":1}]})// yellow
			colors,
		VuoInputData(VuoReal, {"default":1,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) filterOpacity,
		VuoOutputData(VuoImage) mappedImage
)
{
	*mappedImage = VuoImage_mapColors(image, colors, filterOpacity);
}
