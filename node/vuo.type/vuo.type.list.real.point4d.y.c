/**
 * @file
 * vuo.type.list.real.point4d.y node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 4D Point List (0,Y,0,0)",
					  "description": "Creates a list of 4D points using the input real numbers as the Y coordinate, and 0 as the X, Z, and W coordinates.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) y,
	VuoOutputData(VuoList_VuoPoint4d, {"name":"(0,Y,0,0)"}) point4d
)
{
	unsigned long count = VuoListGetCount_VuoReal(y);
	VuoReal *inputs = VuoListGetData_VuoReal(y);
	*point4d = VuoListCreateWithCount_VuoPoint4d(count, (VuoPoint4d){0,0,0,0});
	VuoPoint4d *outputs = VuoListGetData_VuoPoint4d(*point4d);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint4d){0, inputs[i], 0, 0};
}
