/**
 * @file
 * vuo.type.list.point2d.point3d.x node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 2D Point List to 3D Point List (X,Y,0)",
					  "description": "Expands a list of 2D points (X,Y) to a list of 3D points (X,Y,0).",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint2d, {"name":"(X,Y)"}) xy,
	VuoOutputData(VuoList_VuoPoint3d, {"name":"(X,Y,0)"}) xyz
)
{
	unsigned long count = VuoListGetCount_VuoPoint2d(xy);
	VuoPoint2d *inputs = VuoListGetData_VuoPoint2d(xy);
	*xyz = VuoListCreateWithCount_VuoPoint3d(count, (VuoPoint3d){0,0,0});
	VuoPoint3d *outputs = VuoListGetData_VuoPoint3d(*xyz);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint3d){inputs[i].x, inputs[i].y, 0};
}
