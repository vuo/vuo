/**
 * @file
 * vuo.type.list.real.point3d.xyz node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 3D Point List (X,X,X)",
					  "description": "Creates a list of 3D points using the input real numbers as the X, Y, and Z coordinates.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) x,
	VuoOutputData(VuoList_VuoPoint3d, {"name":"(X,X,X)"}) point3d
)
{
	unsigned long count = VuoListGetCount_VuoReal(x);
	VuoReal *items = VuoListGetData_VuoReal(x);
	*point3d = VuoListCreateWithCount_VuoPoint3d(count, (VuoPoint3d){0,0,0});
	VuoPoint3d *outputItems = VuoListGetData_VuoPoint3d(*point3d);
	for (unsigned long i = 0; i < count; ++i)
	{
		VuoReal v = items[i];
		outputItems[i] = (VuoPoint3d){v,v,v};
	}
}
