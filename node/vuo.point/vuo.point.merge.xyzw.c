/**
 * @file
 * vuo.point.merge.xyzw node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Merge XYZW Lists",
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
		VuoInputData(VuoList_VuoReal) w,
		VuoOutputData(VuoList_VuoPoint4d) points
)
{
	unsigned long xCount = VuoListGetCount_VuoReal(x);
	unsigned long yCount = VuoListGetCount_VuoReal(y);
	unsigned long zCount = VuoListGetCount_VuoReal(z);
	unsigned long wCount = VuoListGetCount_VuoReal(w);
	unsigned long pointCount = MAX(MAX(MAX(xCount, yCount), zCount), wCount);

	*points = VuoListCreateWithCount_VuoPoint4d(pointCount, (VuoPoint4d){0,0,0,0});
	VuoPoint4d *pointValues = VuoListGetData_VuoPoint4d(*points);

	VuoReal *xValues = VuoListGetData_VuoReal(x);
	VuoReal *yValues = VuoListGetData_VuoReal(y);
	VuoReal *zValues = VuoListGetData_VuoReal(z);
	VuoReal *wValues = VuoListGetData_VuoReal(w);

	VuoReal xPrior  = NAN;
	VuoReal xPrior2 = NAN;
	VuoReal yPrior  = NAN;
	VuoReal yPrior2 = NAN;
	VuoReal zPrior  = NAN;
	VuoReal zPrior2 = NAN;
	VuoReal wPrior  = NAN;
	VuoReal wPrior2 = NAN;
	for (unsigned long i = 1; i <= pointCount; ++i)
	{
		VuoReal ix = 0;
		VuoReal iy = 0;
		VuoReal iz = 0;
		VuoReal iw = 0;

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

		if (i <= wCount)
			iw = wValues[i-1];
		else if (!isnan(wPrior2))
			iw = wPrior*2 - wPrior2;
		else if (!isnan(wPrior))
			iw = wPrior;

		pointValues[i-1] = (VuoPoint4d){ix, iy, iz, iw};

		xPrior2 = xPrior;
		yPrior2 = yPrior;
		zPrior2 = zPrior;
		wPrior2 = wPrior;
		xPrior = ix;
		yPrior = iy;
		zPrior = iz;
		wPrior = iw;
	}
}
