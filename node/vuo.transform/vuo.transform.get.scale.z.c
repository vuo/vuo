/**
 * @file
 * vuo.transform.get.scale.z node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Get Transform Z Scale",
	"keywords" : [ ],
	"version" : "1.0.0",
	"node": {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent
(
	VuoInputData(VuoTransform) transform,
	VuoOutputData(VuoReal) zScale
)
{
	*zScale = transform.scale.z;
}
