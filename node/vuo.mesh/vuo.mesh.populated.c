/**
 * @file
 * vuo.mesh.populated node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoMesh.h"

VuoModuleMetadata({
					  "title" : "Is Mesh Populated",
					  "keywords" : [ "3d", "scene", "vertices", "non-empty", "nonempty" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoMesh) mesh,
		VuoOutputData(VuoBoolean) populated
)
{
	*populated = VuoMesh_isPopulated(mesh);
}
