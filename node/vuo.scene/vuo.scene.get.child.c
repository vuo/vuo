/**
 * @file
 * vuo.scene.get.child node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoList_VuoSceneObject.h"

VuoModuleMetadata({
					  "title" : "Get Child Objects",
					  "keywords" : [ "3D" ],
					  "version" : "1.0.0",
				  });

void nodeEvent
(
		VuoInputData(VuoSceneObject) object,
		VuoOutputData(VuoList_VuoSceneObject) childObjects
)
{
	*childObjects = object.childObjects;
}

