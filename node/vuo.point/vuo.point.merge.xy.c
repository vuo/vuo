/**
 * @file
 * vuo.point.merge.xy node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Merge XY Lists",
					 "keywords" : [ "append", "join", "together", "combine", "collate", "interleave", "match",
						 /* Swift */ "zip"
					 ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ "DrawPointsAlongCurve.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoReal) x,
		VuoInputData(VuoList_VuoReal) y,
		VuoOutputData(VuoList_VuoPoint2d) points
)
{
	unsigned long xCount = VuoListGetCount_VuoReal(x);
	unsigned long yCount = VuoListGetCount_VuoReal(y);
	unsigned long pointCount = MAX(xCount, yCount);

	*points = VuoListCreateWithCount_VuoPoint2d(pointCount, (VuoPoint2d){0,0});
	VuoPoint2d *pointValues = VuoListGetData_VuoPoint2d(*points);

	VuoReal *xValues = VuoListGetData_VuoReal(x);
	VuoReal *yValues = VuoListGetData_VuoReal(y);

	VuoReal xPrior  = NAN;
	VuoReal xPrior2 = NAN;
	VuoReal yPrior  = NAN;
	VuoReal yPrior2 = NAN;
	for (unsigned long i = 1; i <= pointCount; ++i)
	{
		VuoReal ix = 0;
		VuoReal iy = 0;

		if (i <= xCount)
			ix = xValues[i-1];
		else if (!isnan(xPrior2))
			ix = xPrior*2 - xPrior2;
		else if (!isnan(xPrior))
			ix = xPrior;

		if (i <= yCount)
			iy = yValues[i-1];
		else if (!isnan(yPrior2))
			iy = yPrior*2 - yPrior2;
		else if (!isnan(yPrior))
			iy = yPrior;

		pointValues[i-1] = (VuoPoint2d){ix, iy};

		xPrior2 = xPrior;
		yPrior2 = yPrior;
		xPrior = ix;
		yPrior = iy;
	}
}
