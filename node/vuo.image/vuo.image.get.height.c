/**
 * @file
 * vuo.image.get.height node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Get Image Height",
					 "keywords" : [ "dimensions", "size" ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoImage) image,
	VuoOutputData(VuoInteger) height
)
{
	if (!image)
		return;

	*height = image->pixelsHigh;
}
