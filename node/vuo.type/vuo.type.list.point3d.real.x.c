/**
 * @file
 * vuo.type.list.point3d.real.x node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 3D Point List to Real List (X)",
					  "description": "Creates a list of real numbers using the X coordinate of the input list of 3D points.",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint3d, {"name":"(X,Y,Z)"}) point3d,
	VuoOutputData(VuoList_VuoReal) x
)
{
	unsigned long count = VuoListGetCount_VuoPoint3d(point3d);
	VuoPoint3d *inputs = VuoListGetData_VuoPoint3d(point3d);
	*x = VuoListCreateWithCount_VuoReal(count, 0);
	VuoReal *outputs = VuoListGetData_VuoReal(*x);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = inputs[i].x;
}
