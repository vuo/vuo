/**
 * @file
 * vuo.type.list.point4d.real.x node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 4D Point List to Real List (X)",
					  "description": "Creates a list of real numbers using the X coordinate of the input list of 4D points.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint4d, {"name":"(X,Y,Z,W)"}) point4d,
	VuoOutputData(VuoList_VuoReal) x
)
{
	unsigned long count = VuoListGetCount_VuoPoint4d(point4d);
	VuoPoint4d *inputs = VuoListGetData_VuoPoint4d(point4d);
	*x = VuoListCreateWithCount_VuoReal(count, 0);
	VuoReal *outputs = VuoListGetData_VuoReal(*x);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = inputs[i].x;
}
