/**
 * @file
 * vuo.layer.copy node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Copy Layer",
					 "keywords" : [ "duplicate", "array", "instance", "instantiate", "populate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoList_VuoTransform2d) transforms,
	VuoOutputData(VuoList_VuoLayer) copies
)
{
	*copies = VuoListCreate_VuoLayer();
	unsigned long transform_length = VuoListGetCount_VuoTransform2d(transforms);

	for(int i = 0; i < transform_length; i++)
	{
		VuoLayer o;
		o.sceneObject = layer.sceneObject;
		o.sceneObject.transform = VuoTransform_makeFrom2d(VuoListGetValueAtIndex_VuoTransform2d(transforms, i+1));

		VuoListAppendValue_VuoLayer(*copies, o);
	}
}
