﻿/**
 * @file
 * vuo.type.list.real.point3d.y node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title": "Convert Real List to 3D Point List (0,Y,0)",
					  "description": "Creates a list of 3D points using the input real numbers as the Y coordinate, and 0 as the X and Z coordinates.",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) y,
	VuoOutputData(VuoList_VuoPoint3d, {"name":"(0,Y,0)"}) point3d
)
{
	unsigned long count = VuoListGetCount_VuoReal(y);
	VuoReal *inputs = VuoListGetData_VuoReal(y);
	*point3d = VuoListCreateWithCount_VuoPoint3d(count, (VuoPoint3d){0,0,0});
	VuoPoint3d *outputs = VuoListGetData_VuoPoint3d(*point3d);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint3d){0, inputs[i], 0};
}
