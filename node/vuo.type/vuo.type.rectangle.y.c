/**
 * @file
 * vuo.type.rectangle.y node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Convert Rectangle to Real (Center Y)",
	"description": "Outputs the rectangle's center Y coordinate.",
	"keywords": [ ],
	"version": "1.0.0",
});

void nodeEvent(
	VuoInputData(VuoRectangle) rectangle,
	VuoOutputData(VuoReal) y)
{
	*y = rectangle.center.y;
}
