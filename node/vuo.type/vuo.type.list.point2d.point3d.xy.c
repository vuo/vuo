/**
 * @file
 * vuo.type.list.point2d.point3d.x node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 2D Point List to 3D Point List",
					  "description": "Expands a list of 2D points (X,Y) to a list of 3D points (X,Y,0).",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint2d) xy,
	VuoOutputData(VuoList_VuoPoint3d) xyz
)
{
	*xyz = VuoListCreate_VuoPoint3d();
	unsigned long count = VuoListGetCount_VuoPoint2d(xy);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoPoint2d p = VuoListGetValue_VuoPoint2d(xy, i);
		VuoListAppendValue_VuoPoint3d(*xyz, VuoPoint3d_make(p.x, p.y, 0));
	}
}
