/**
 * @file
 * vuo.type.list.real.point3d.z node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 3D Point List (0,0,Z)",
					  "description": "Creates a list of 3D points using the input real numbers as the Z coordinate, and 0 as the X and Y coordinates.",
					  "version": "1.0.1"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) z,
	VuoOutputData(VuoList_VuoPoint3d, {"name":"(0,0,Z)"}) point3d
)
{
	*point3d = VuoListCreate_VuoPoint3d();
	unsigned long count = VuoListGetCount_VuoReal(z);
	for (unsigned long i = 1; i <= count; ++i)
		VuoListAppendValue_VuoPoint3d(*point3d, VuoPoint3d_make(0, 0, VuoListGetValue_VuoReal(z, i)));
}
