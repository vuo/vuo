/**
 * @file
 * vuo.type.list.point3d.point2d.xy node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 3D Point List to 2D Point List (X,Y)",
					  "description": "Outputs a list of just the (X,Y) part of a list of 3D points (X,Y,Z).",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint3d, {"name":"(X,Y,Z)"}) xyz,
	VuoOutputData(VuoList_VuoPoint2d, {"name":"(X,Y)"}) xy
)
{
	unsigned long count = VuoListGetCount_VuoPoint3d(xyz);
	VuoPoint3d *inputs = VuoListGetData_VuoPoint3d(xyz);
	*xy = VuoListCreateWithCount_VuoPoint2d(count, (VuoPoint2d){0,0});
	VuoPoint2d *outputs = VuoListGetData_VuoPoint2d(*xy);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint2d){inputs[i].x, inputs[i].y};
}
