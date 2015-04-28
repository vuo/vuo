/**
 * @file
 * vuo.scene.copy node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Copy 3D Object",
					  "keywords" : [ "duplicate", "array", "instance", "instantiate", "populate", "replicate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoList_VuoTransform) transforms,
	VuoOutputData(VuoList_VuoSceneObject) copies
)
{
	*copies = VuoListCreate_VuoSceneObject();
	for(int i = 0; i < VuoListGetCount_VuoTransform(transforms); i++)
	{
		VuoTransform transform = VuoListGetValueAtIndex_VuoTransform(transforms, i+1);

		VuoListAppendValue_VuoSceneObject(*copies, VuoSceneObject_make(
			object.verticesList,
			object.shader,
			transform,
			object.childObjects
			));
	}
}
