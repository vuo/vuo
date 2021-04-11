/**
 * @file
 * vuo.rectangle.make node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Make Rectangle",
	"keywords" : [
		"xy", "cartesian", "euler", "coordinates", "vector", "2D",
		"box", "square",
	],
	"version" : "1.0.0",
	"node" : {
		"exampleCompositions" : []
	}
});

void nodeEvent(
	VuoInputData(VuoPoint2d, { "default" : [ 0.0, 0.0 ] }) center,
	VuoInputData(VuoPoint2d, { "default" : [ 1.0, 1.0 ] }) size,
	VuoOutputData(VuoRectangle) rectangle)
{
	rectangle->center = center;
	rectangle->size = size;
}
