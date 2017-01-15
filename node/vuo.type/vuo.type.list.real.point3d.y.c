/**
 * @file
 * vuo.type.list.real.point3d.y node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 3D Point List (0,Y,0)",
					  "description": "Creates a list of 3D points using the input real numbers as the Y coordinate, and 0 as the X and Z coordinates.",
					  "version": "1.0.1"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) y,
	VuoOutputData(VuoList_VuoPoint3d, {"name":"(0,Y,0)"}) point3d
)
{
	*point3d = VuoListCreate_VuoPoint3d();
	unsigned long count = VuoListGetCount_VuoReal(y);
	for (unsigned long i = 1; i <= count; ++i)
		VuoListAppendValue_VuoPoint3d(*point3d, VuoPoint3d_make(0, VuoListGetValue_VuoReal(y, i), 0));
}
