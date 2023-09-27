/**
 * @file
 * vuo.point.make.grid.3d node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Make Points in 3D Grid",
					  "keywords" : [
						  "math", "shape", "easing", "linear", "quadratic", "cubic", "circular", "exponential", "logarithmic", "interpolate",
						  "matrix", "lattice", "cube", "prism", "rectangular prism", "box",
					  ],
					  "version" : "1.0.1",
					  "node" : {
						  "exampleCompositions" : [ "TravelThroughInfiniteGrid.vuo" ]
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
		VuoInputData(VuoRange, {"default":{"minimum":-1,"maximum":1},
								"suggestedMin":{"minimum":-1,"maximum":-1},
								"suggestedMax":{"minimum":1,"maximum":1},
								"requireMin":true, "requireMax":true}) zRange,
		VuoInputData(VuoCurve, {"default":"linear"}) xCurve,
		VuoInputData(VuoCurve, {"default":"linear"}) yCurve,
		VuoInputData(VuoCurve, {"default":"linear"}) zCurve,
		VuoInputData(VuoCurveEasing, {"default":"in"}) xEasing,
		VuoInputData(VuoCurveEasing, {"default":"in"}) yEasing,
		VuoInputData(VuoCurveEasing, {"default":"in"}) zEasing,
		VuoInputData(VuoInteger, {"default":8, "suggestedMin":1, "suggestedMax":32}) xPointCount,
		VuoInputData(VuoInteger, {"default":8, "suggestedMin":1, "suggestedMax":32}) yPointCount,
		VuoInputData(VuoInteger, {"default":8, "suggestedMin":1, "suggestedMax":32}) zPointCount,
		VuoOutputData(VuoList_VuoPoint3d) points
)
{
	if (xPointCount * yPointCount * zPointCount <= 0)
	{
		*points = NULL;
		return;
	}

	*points = VuoListCreateWithCount_VuoPoint3d(xPointCount * yPointCount * zPointCount, VuoPoint3d_make(0, 0, 0));
	VuoPoint3d *pointsArr = VuoListGetData_VuoPoint3d(*points);

	for (int i = 0; i < xPointCount; ++i)
	{
		float x = VuoReal_curve(i, xRange.minimum, xRange.maximum, xPointCount-1, xCurve, xEasing, VuoLoopType_None);
		for (int j = 0; j < yPointCount; ++j)
		{
			float y = VuoReal_curve(j, yRange.minimum, yRange.maximum, yPointCount-1, yCurve, yEasing, VuoLoopType_None);
			for (int k = 0; k < zPointCount; ++k)
			{
				float z = VuoReal_curve(k, zRange.minimum, zRange.maximum, zPointCount-1, zCurve, zEasing, VuoLoopType_None);
				pointsArr[i*yPointCount*zPointCount + j*zPointCount + k] = VuoPoint3d_make(x, y, z);
			}
		}
	}
}
