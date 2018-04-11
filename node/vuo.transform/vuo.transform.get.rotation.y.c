/**
 * @file
 * vuo.transform.get.rotation.y node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Get Transform Y Rotation",
	"keywords" : [ ],
	"version" : "1.0.0",
	"node": {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent
(
	VuoInputData(VuoTransform) transform,
	VuoOutputData(VuoReal) yRotation
)
{
	*yRotation = VuoTransform_getEuler(transform).y * 180./M_PI;
}
