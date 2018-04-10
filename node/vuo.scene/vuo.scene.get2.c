/**
 * @file
 * vuo.scene.get2 node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneObjectType.h"

VuoModuleMetadata({
					 "title" : "Get 3D Object Values",
					   "keywords" : [ "information", "children", "transform", "type" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoSceneObject) object,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoTransform) transform,
		VuoOutputData(VuoSceneObjectType) type,
		VuoOutputData(VuoList_VuoSceneObject) childObjects
)
{
	*name = VuoText_make(object.name);
	*transform = object.transform;
	*type = VuoSceneObjectType_makeFromSubtype(object.type);
	*childObjects = object.childObjects;
}
