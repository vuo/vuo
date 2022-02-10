/**
 * @file
 * vuo.type.rectangle.center node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Convert Rectangle to 2D Point (Center)",
	"description": "Outputs the rectangle's center point.",
	"keywords": [ ],
	"version": "1.0.0",
});

void nodeEvent(
	VuoInputData(VuoRectangle) rectangle,
	VuoOutputData(VuoPoint2d) center)
{
	*center = rectangle.center;
}
