/**
 * @file
 * vuo.point.merge.xy node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Merge XY Lists",
					 "keywords" : [ "append", "join", "together", "combine", "collate", "interleave", "match" ],
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
	*points = VuoListCreate_VuoPoint2d();

	unsigned long xCount = VuoListGetCount_VuoReal(x);
	unsigned long yCount = VuoListGetCount_VuoReal(y);
	VuoReal xPrior  = NAN;
	VuoReal xPrior2 = NAN;
	VuoReal yPrior  = NAN;
	VuoReal yPrior2 = NAN;
	for (unsigned long i = 1; i <= MAX(xCount, yCount); ++i)
	{
		VuoReal ix = 0;
		VuoReal iy = 0;

		if (i <= xCount)
			ix = VuoListGetValue_VuoReal(x, i);
		else if (!isnan(xPrior2))
			ix = xPrior*2 - xPrior2;
		else if (!isnan(xPrior))
			ix = xPrior;

		if (i <= yCount)
			iy = VuoListGetValue_VuoReal(y, i);
		else if (!isnan(yPrior2))
			iy = yPrior*2 - yPrior2;
		else if (!isnan(yPrior))
			iy = yPrior;

		VuoListAppendValue_VuoPoint2d(*points, VuoPoint2d_make(ix, iy));

		xPrior2 = xPrior;
		yPrior2 = yPrior;
		xPrior = ix;
		yPrior = iy;
	}
}
