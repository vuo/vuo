/**
 * @file
 * vuo.type.list.real.point2d.xy node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 2D Point List (X,X)",
					  "description": "Creates a list of 2D points using the input real numbers as the X and Y coordinates.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) x,
	VuoOutputData(VuoList_VuoPoint2d, {"name":"(X,X)"}) point2d
)
{
	unsigned long count = VuoListGetCount_VuoReal(x);
	VuoReal *items = VuoListGetData_VuoReal(x);
	*point2d = VuoListCreateWithCount_VuoPoint2d(count, (VuoPoint2d){0,0});
	VuoPoint2d *outputItems = VuoListGetData_VuoPoint2d(*point2d);
	for (unsigned long i = 0; i < count; ++i)
	{
		VuoReal v = items[i];
		outputItems[i] = (VuoPoint2d){v,v};
	}
}
