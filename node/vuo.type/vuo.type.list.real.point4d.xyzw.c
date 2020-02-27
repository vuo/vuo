/**
 * @file
 * vuo.type.list.real.point4d.xyzw node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 4D Point List (X,X,X,X)",
					  "description": "Creates a list of 4D points using the input real numbers as the X, Y, Z, and W coordinates.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) x,
	VuoOutputData(VuoList_VuoPoint4d, {"name":"(X,X,X,X)"}) point4d
)
{
	unsigned long count = VuoListGetCount_VuoReal(x);
	VuoReal *items = VuoListGetData_VuoReal(x);
	*point4d = VuoListCreateWithCount_VuoPoint4d(count, (VuoPoint4d){0,0,0,0});
	VuoPoint4d *outputItems = VuoListGetData_VuoPoint4d(*point4d);
	for (unsigned long i = 0; i < count; ++i)
	{
		VuoReal v = items[i];
		outputItems[i] = (VuoPoint4d){v,v,v,v};
	}
}
