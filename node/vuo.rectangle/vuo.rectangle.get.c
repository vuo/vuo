/**
 * @file
 * vuo.rectangle.get node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Get Rectangle Values",
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
	VuoInputData(VuoRectangle, { "default" : { "center" : [0.0,0.0], "size" : [1.0,1.0] } }) rectangle,
	VuoOutputData(VuoPoint2d) center,
	VuoOutputData(VuoPoint2d) size)
{
	*center = rectangle.center;
	*size = rectangle.size;
}
