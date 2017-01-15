/**
 * @file
 * vuo.transform.combine node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Combine Transforms",
					 "keywords" : [ "composite", "product", "multiply", "merge" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "TiltAndOrbitCube.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoTransform) transforms,
		VuoOutputData(VuoTransform) composite
)
{
	*composite = VuoTransform_makeIdentity();

	unsigned long transformCount = VuoListGetCount_VuoTransform(transforms);
	for (unsigned long i = 1; i <= transformCount; ++i)
		*composite = VuoTransform_composite(*composite, VuoListGetValue_VuoTransform(transforms, i));
}
