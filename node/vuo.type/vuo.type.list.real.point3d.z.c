/**
 * @file
 * vuo.type.list.real.point3d.z node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 3D Point List (0,0,Z)",
					  "description": "Creates a list of 3D points using the input real numbers as the Z coordinate, and 0 as the X and Y coordinates.",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) z,
	VuoOutputData(VuoList_VuoPoint3d, {"name":"(0,0,Z)"}) point3d
)
{
	unsigned long count = VuoListGetCount_VuoReal(z);
	VuoReal *inputs = VuoListGetData_VuoReal(z);
	*point3d = VuoListCreateWithCount_VuoPoint3d(count, (VuoPoint3d){0,0,0});
	VuoPoint3d *outputs = VuoListGetData_VuoPoint3d(*point3d);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint3d){0, 0, inputs[i]};
}
