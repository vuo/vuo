/**
 * @file
 * vuo.type.list.point3d.point2d.xy node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 3D Point List to 2D Point List (X,Y)",
					  "description": "Outputs a list of just the (X,Y) part of a list of 3D points (X,Y,Z).",
					  "version": "1.0.1"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint3d, {"name":"(X,Y,Z)"}) xyz,
	VuoOutputData(VuoList_VuoPoint2d, {"name":"(X,Y)"}) xy
)
{
	*xy = VuoListCreate_VuoPoint2d();
	unsigned long count = VuoListGetCount_VuoPoint3d(xyz);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoPoint3d p = VuoListGetValue_VuoPoint3d(xyz, i);
		VuoListAppendValue_VuoPoint2d(*xy, VuoPoint2d_make(p.x, p.y));
	}
}
