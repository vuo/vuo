/**
 * @file
 * vuo.scene.copy node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Copy 3D Object (Transform)",
					 "keywords" : [ "duplicate", "clone", "repeat", "replicate", "array", "instance", "instantiate", "populate" ],
					 "version" : "2.0.2",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoList_VuoTransform) transforms,
	VuoOutputData(VuoSceneObject) copies
)
{
	*copies = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeIdentity());

	for(int i = 0; i < VuoListGetCount_VuoTransform(transforms); i++)
	{
		VuoTransform transform = VuoListGetValue_VuoTransform(transforms, i+1);

		VuoSceneObject so = VuoSceneObject_copy(object);
		VuoSceneObject_transform(so, transform);
		VuoListAppendValue_VuoSceneObject(VuoSceneObject_getChildObjects(*copies), so);
	}
}
