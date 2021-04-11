/**
 * @file
 * vuo.scene.populated node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoSceneObject.h"

VuoModuleMetadata({
					 "title" : "Is 3D Object Populated",
					 "keywords" : [ "mesh", "model", "vertices", "empty", "non-empty", "nonempty" ],
					 "version" : "1.0.0",
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoSceneObject) object,
		VuoOutputData(VuoBoolean) populated
)
{
	*populated = VuoSceneObject_isPopulated(object);
}
