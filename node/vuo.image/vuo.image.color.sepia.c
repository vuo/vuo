/**
 * @file
 * vuo.image.color.sepia node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Sepia Image",
					 "keywords" : [
						 "tone", "old-fashioned", "daguerreotype", "vintage", "adjust",
						 "filter"
					 ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "SaveSepiaImage.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.01}) amount,
		VuoOutputData(VuoImage) sepiaImage
)
{
	VuoList_VuoColor colors = VuoListCreate_VuoColor();
	VuoRetain(colors);
	VuoListAppendValue_VuoColor(colors, (VuoColor){0,0,0,1});
	VuoListAppendValue_VuoColor(colors, (VuoColor){.55,.45,.36,1});
	VuoListAppendValue_VuoColor(colors, (VuoColor){1,1,1,1});
	*sepiaImage = VuoImage_mapColors(image, colors, amount);
	VuoRelease(colors);
}
