/**
 * @file
 * vuo.type.list.real.point2d.x node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 2D Point List (X,0)",
					  "description": "Creates a list of 2D points using the input real numbers as the X coordinate, and 0 as the Y coordinate.",
					  "version": "1.0.1"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) x,
	VuoOutputData(VuoList_VuoPoint2d, {"name":"(X,0)"}) point2d
)
{
	*point2d = VuoListCreate_VuoPoint2d();
	unsigned long count = VuoListGetCount_VuoReal(x);
	for (unsigned long i = 1; i <= count; ++i)
		VuoListAppendValue_VuoPoint2d(*point2d, VuoPoint2d_make(VuoListGetValue_VuoReal(x, i), 0));
}
