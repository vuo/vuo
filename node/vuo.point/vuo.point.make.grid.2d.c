/**
 * @file
 * vuo.point.make.grid.2d node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Make Points in 2D Grid",
					  "keywords" : [
						  "math", "shape", "easing", "linear", "quadratic", "cubic", "circular", "exponential", "logarithmic", "interpolate",
						  "matrix", "lattice", "plane", "square", "rectangle", "quadrilateral",
					  ],
					  "version" : "1.0.1",
					  "node" : {
						  "exampleCompositions" : [ "DisplayGridOfObjects.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoRange, {"default":{"minimum":-1,"maximum":1},
								"suggestedMin":{"minimum":-1,"maximum":-1},
								"suggestedMax":{"minimum":1,"maximum":1},
								"requireMin":true, "requireMax":true}) xRange,
		VuoInputData(VuoRange, {"default":{"minimum":-1,"maximum":1},
								"suggestedMin":{"minimum":-1,"maximum":-1},
								"suggestedMax":{"minimum":1,"maximum":1},
								"requireMin":true, "requireMax":true}) yRange,
		VuoInputData(VuoCurve, {"default":"linear"}) xCurve,
		VuoInputData(VuoCurve, {"default":"linear"}) yCurve,
		VuoInputData(VuoCurveEasing, {"default":"in"}) xEasing,
		VuoInputData(VuoCurveEasing, {"default":"in"}) yEasing,
		VuoInputData(VuoInteger, {"default":16, "suggestedMin":1, "suggestedMax":256}) xPointCount,
		VuoInputData(VuoInteger, {"default":16, "suggestedMin":1, "suggestedMax":256}) yPointCount,
		VuoOutputData(VuoList_VuoPoint2d) points
)
{
	if (xPointCount * yPointCount <= 0)
	{
		*points = NULL;
		return;
	}

	*points = VuoListCreateWithCount_VuoPoint2d(xPointCount * yPointCount, VuoPoint2d_make(0, 0));
	VuoPoint2d *pointsArr = VuoListGetData_VuoPoint2d(*points);

	for (int i = 0; i < xPointCount; ++i)
	{
		float x = VuoReal_curve(i, xRange.minimum, xRange.maximum, xPointCount-1, xCurve, xEasing, VuoLoopType_None);
		for (int j = 0; j < yPointCount; ++j)
		{
			float y = VuoReal_curve(j, yRange.minimum, yRange.maximum, yPointCount-1, yCurve, yEasing, VuoLoopType_None);
			pointsArr[i*yPointCount + j] = VuoPoint2d_make(x, y);
		}
	}
}
