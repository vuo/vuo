/**
 * @file
 * vuo.type.list.real.point2d.x node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title": "Convert Real List to 2D Point List (X,0)",
					  "description": "Creates a list of 2D points using the input real numbers as the X coordinate, and 0 as the Y coordinate.",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) x,
	VuoOutputData(VuoList_VuoPoint2d, {"name":"(X,0)"}) point2d
)
{
	unsigned long count = VuoListGetCount_VuoReal(x);
	VuoReal *inputs = VuoListGetData_VuoReal(x);
	*point2d = VuoListCreateWithCount_VuoPoint2d(count, (VuoPoint2d){0,0});
	VuoPoint2d *outputs = VuoListGetData_VuoPoint2d(*point2d);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint2d){inputs[i], 0};
}
