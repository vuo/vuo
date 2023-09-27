/**
 * @file
 * vuo.layer.bounds node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
	"title" : "Get Layer Bounds",
	"keywords": [
		"box", "rectangle",
		"aabb", "axis", "collider", "collision",
		"size", "dimensions", "measurements", "geometry", "bounding", "boundaries", "length",
	],
	"version" : "1.0.0",
	"node": {
		"exampleCompositions" : [ "StretchOvalWithBounds.vuo" ]
	}
});

void nodeEvent
(
	VuoInputData(VuoLayer) layer,
	VuoOutputData(VuoPoint2d) center,
	VuoOutputData(VuoReal) width,
	VuoOutputData(VuoReal) height
)
{
	VuoRectangle bounds = VuoLayer_getBoundingRectangle(layer, -1, -1, 1);

	*center = bounds.center;
	*width = bounds.size.x;
	*height = bounds.size.y;
}
