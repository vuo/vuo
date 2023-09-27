/**
 * @file
 * vuo.type.rectangle.size node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Convert Rectangle to 2D Point (Size)",
	"description": "Outputs the rectangle's size as a 2D point.",
	"keywords": [ ],
	"version": "1.0.0",
});

void nodeEvent(
	VuoInputData(VuoRectangle) rectangle,
	VuoOutputData(VuoPoint2d) size)
{
	*size = rectangle.size;
}
