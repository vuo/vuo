/**
 * @file
 * vuo.type.image.layer node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "../vuo.layer/VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Convert Image to Layer",
					 "keywords" : [ "billboard", "sprite", "scaled",
						 "stretch", "fill", "shrink", "blow up", "enlarge", "magnify" ],
					 "version" : "1.0.0",
					 "node": {
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_make(NULL, image, VuoPoint2d_make(0,0), 0, 2, 1);
}
