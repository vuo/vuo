/**
 * @file
 * vuo.scene.combine.group node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Combine 3D Objects (Group)",
					 "keywords" : [ "scenegraph", "join", "together", "merge" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoSceneObject) objects,
		VuoOutputData(VuoSceneObject) combinedObject
)
{
	*combinedObject = VuoSceneObject_makeGroup(objects, VuoTransform_makeIdentity());
}
