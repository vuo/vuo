/**
 * @file
 * vuo.type.rectangle.width node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Convert Rectangle to Real (Width)",
	"description": "Outputs the rectangle's width.",
	"keywords": [ ],
	"version": "1.0.0",
});

void nodeEvent(
	VuoInputData(VuoRectangle) rectangle,
	VuoOutputData(VuoReal) width)
{
	*width = rectangle.size.x;
}
