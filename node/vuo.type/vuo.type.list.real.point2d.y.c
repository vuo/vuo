/**
 * @file
 * vuo.type.list.real.point2d.y node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 2D Point List (0,Y)",
					  "description": "Creates a list of 2D points using the input real numbers as the Y coordinate, and 0 as the X coordinate.",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) y,
	VuoOutputData(VuoList_VuoPoint2d, {"name":"(0,Y)"}) point2d
)
{
	unsigned long count = VuoListGetCount_VuoReal(y);
	VuoReal *inputs = VuoListGetData_VuoReal(y);
	*point2d = VuoListCreateWithCount_VuoPoint2d(count, (VuoPoint2d){0,0});
	VuoPoint2d *outputs = VuoListGetData_VuoPoint2d(*point2d);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint2d){0, inputs[i]};
}
