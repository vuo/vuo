/**
 * @file
 * vuo.type.list.point2d.real.x node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 2D Point List to Real List (X)",
					  "description": "Creates a list of real numbers using the X coordinate of the input list of 2D points.",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint2d, {"name":"(X,Y)"}) point2d,
	VuoOutputData(VuoList_VuoReal) x
)
{
	unsigned long count = VuoListGetCount_VuoPoint2d(point2d);
	VuoPoint2d *inputs = VuoListGetData_VuoPoint2d(point2d);
	*x = VuoListCreateWithCount_VuoReal(count, 0);
	VuoReal *outputs = VuoListGetData_VuoReal(*x);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = inputs[i].x;
}
