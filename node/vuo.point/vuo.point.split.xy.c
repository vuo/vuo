/**
 * @file
 * vuo.point.split.VuoPoint2d node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Split XY List",
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
	VuoInputData(VuoList_VuoPoint2d) points,
	VuoOutputData(VuoList_VuoReal) x,
	VuoOutputData(VuoList_VuoReal) y
)
{
	unsigned long count = VuoListGetCount_VuoPoint2d(points);
	*x = VuoListCreateWithCount_VuoReal(count, 0);
	*y = VuoListCreateWithCount_VuoReal(count, 0);

	VuoReal *xArr = VuoListGetData_VuoReal(*x);
	VuoReal *yArr = VuoListGetData_VuoReal(*y);
	VuoPoint2d *pointsArr = VuoListGetData_VuoPoint2d(points);

	for (unsigned long i = 0; i < count; ++i)
    {
		xArr[i] = pointsArr[i].x;
		yArr[i] = pointsArr[i].y;
	}
}
