/**
 * @file
 * vuo.layer.copy node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Copy Layer with Transforms",
					 "keywords" : [ "duplicate", "clone", "array", "instance", "instantiate", "populate" ],
					 "version" : "2.0.1",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoList_VuoTransform2d) transforms,
	VuoOutputData(VuoLayer) copies
)
{
	VuoList_VuoLayer layers = VuoListCreate_VuoLayer();
	unsigned long transform_length = VuoListGetCount_VuoTransform2d(transforms);

	for(int i = 0; i < transform_length; i++)
	{
		VuoLayer o;
		o.sceneObject = layer.sceneObject;
		o.sceneObject.transform = VuoTransform_composite(o.sceneObject.transform,
														 VuoTransform_makeFrom2d(VuoListGetValue_VuoTransform2d(transforms, i+1)));

		VuoListAppendValue_VuoLayer(layers, o);
	}

	VuoRetain(layers);
	*copies = VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity());
	VuoRelease(layers);
}
