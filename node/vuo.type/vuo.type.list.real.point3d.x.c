/**
 * @file
 * vuo.type.list.real.point3d.x node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title": "Convert Real List to 3D Point List (X,0,0)",
					  "description": "Creates a list of 3D points using the input real numbers as the X coordinate, and 0 as the Y and Z coordinates.",
					  "version": "1.0.2"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) x,
	VuoOutputData(VuoList_VuoPoint3d, {"name":"(X,0,0)"}) point3d
)
{
	unsigned long count = VuoListGetCount_VuoReal(x);
	VuoReal *inputs = VuoListGetData_VuoReal(x);
	*point3d = VuoListCreateWithCount_VuoPoint3d(count, (VuoPoint3d){0,0,0});
	VuoPoint3d *outputs = VuoListGetData_VuoPoint3d(*point3d);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = (VuoPoint3d){inputs[i], 0, 0};
}
