/**
 * @file
 * vuo.type.rectangle.height node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Convert Rectangle to Real (Height)",
	"description": "Outputs the rectangle's height.",
	"keywords": [ ],
	"version": "1.0.0",
});

void nodeEvent(
	VuoInputData(VuoRectangle) rectangle,
	VuoOutputData(VuoReal) height)
{
	*height = rectangle.size.y;
}
