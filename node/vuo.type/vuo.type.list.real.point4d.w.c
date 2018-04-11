/**
 * @file
 * vuo.type.list.real.point4d.w node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to 4D Point List (0,0,0,W)",
					  "description": "Creates a list of 4D points using the input real numbers as the W coordinate, and 0 as the X, Y, and Z coordinates.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) w,
	VuoOutputData(VuoList_VuoPoint4d, {"name":"(0,0,0,W)"}) point4d
)
{
	unsigned long count = VuoListGetCount_VuoReal(w);
	VuoReal *inputs = VuoListGetData_VuoReal(w);
	*point4d = VuoListCreateWithCount_VuoPoint4d(count, (VuoPoint4d){0,0,0,0});
	VuoPoint4d *outputs = VuoListGetData_VuoPoint4d(*point4d);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint4d){0, 0, 0, inputs[i]};
}
