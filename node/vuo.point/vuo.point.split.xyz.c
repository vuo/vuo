/**
 * @file
 * vuo.point.split.VuoPoint3d node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Split XYZ List",
	"keywords": [
		"divide", "separate", "disjoin", "part", "piece", "component", "coordinate", "channel",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint3d) points,
	VuoOutputData(VuoList_VuoReal) x,
	VuoOutputData(VuoList_VuoReal) y,
	VuoOutputData(VuoList_VuoReal) z
)
{
	unsigned long count = VuoListGetCount_VuoPoint3d(points);
	*x = VuoListCreateWithCount_VuoReal(count, 0);
	*y = VuoListCreateWithCount_VuoReal(count, 0);
	*z = VuoListCreateWithCount_VuoReal(count, 0);

	VuoReal *xArr = VuoListGetData_VuoReal(*x);
	VuoReal *yArr = VuoListGetData_VuoReal(*y);
	VuoReal *zArr = VuoListGetData_VuoReal(*z);
	VuoPoint3d *pointsArr = VuoListGetData_VuoPoint3d(points);

	for (unsigned long i = 0; i < count; ++i)
	{
		xArr[i] = pointsArr[i].x;
		yArr[i] = pointsArr[i].y;
		zArr[i] = pointsArr[i].z;
	}
}
