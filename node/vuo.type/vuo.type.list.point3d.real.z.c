/**
 * @file
 * vuo.type.list.point3d.real.x node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 3D Point List to Real List (Z)",
					  "description": "Creates a list of real numbers using the Z coordinate of the input list of 3D points.",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint3d, {"name":"(X,Y,Z)"}) point3d,
	VuoOutputData(VuoList_VuoReal) z
)
{
	unsigned long count = VuoListGetCount_VuoPoint3d(point3d);
	VuoPoint3d *inputs = VuoListGetData_VuoPoint3d(point3d);
	*z = VuoListCreateWithCount_VuoReal(count, 0);
	VuoReal *outputs = VuoListGetData_VuoReal(*z);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = inputs[i].z;
}
