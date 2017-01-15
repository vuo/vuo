/**
 * @file
 * vuo.point.merge.xyz node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Merge XYZ Lists",
					 "keywords" : [ "append", "join", "together", "combine", "collate", "interleave", "match",
						 /* Swift */ "zip"
					 ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoReal) x,
		VuoInputData(VuoList_VuoReal) y,
		VuoInputData(VuoList_VuoReal) z,
		VuoOutputData(VuoList_VuoPoint3d) points
)
{
	unsigned long xCount = VuoListGetCount_VuoReal(x);
	unsigned long yCount = VuoListGetCount_VuoReal(y);
	unsigned long zCount = VuoListGetCount_VuoReal(z);
	unsigned long pointCount = MAX(MAX(xCount, yCount), zCount);

	*points = VuoListCreateWithCount_VuoPoint3d(pointCount, (VuoPoint3d){0,0,0});
	VuoPoint3d *pointValues = VuoListGetData_VuoPoint3d(*points);

	VuoReal *xValues = VuoListGetData_VuoReal(x);
	VuoReal *yValues = VuoListGetData_VuoReal(y);
	VuoReal *zValues = VuoListGetData_VuoReal(z);

	VuoReal xPrior  = NAN;
	VuoReal xPrior2 = NAN;
	VuoReal yPrior  = NAN;
	VuoReal yPrior2 = NAN;
	VuoReal zPrior  = NAN;
	VuoReal zPrior2 = NAN;
	for (unsigned long i = 1; i <= pointCount; ++i)
	{
		VuoReal ix = 0;
		VuoReal iy = 0;
		VuoReal iz = 0;

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

		if (i <= zCount)
			iz = zValues[i-1];
		else if (!isnan(zPrior2))
			iz = zPrior*2 - zPrior2;
		else if (!isnan(zPrior))
			iz = zPrior;

		pointValues[i-1] = (VuoPoint3d){ix, iy, iz};

		xPrior2 = xPrior;
		yPrior2 = yPrior;
		zPrior2 = zPrior;
		xPrior = ix;
		yPrior = iy;
		zPrior = iz;
	}
}
