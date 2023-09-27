/**
 * @file
 * vuo.image.get.size node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Get Image Size",
					 "keywords" : [ "width", "height", "dimensions" ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoImage) image,
	VuoOutputData(VuoInteger) width,
	VuoOutputData(VuoInteger) height
)
{
	if (!image)
		return;

	*width = image->pixelsWide;
	*height = image->pixelsHigh;
}
