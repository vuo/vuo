/**
 * @file
 * vuo.point.make.phyllotaxis node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Make Phyllotaxis Points",
	"keywords": [
		"math", "number", "list",
		"shape", "curve", "circle", "circular", "spiral",
		"phyllotaxy", "botanical", "plant", "rosette", "leaves", "leaf arrangement",
		"natural", "organic",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoPoint2d, {"default":[0,0], "suggestedMin":[-1,-1], "suggestedMax":[1,1]}) center,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":2.0}) radius,
	VuoInputData(VuoInteger, {"name":"Number of Points", "default":256, "suggestedMin":0, "suggestedMax":1024, "suggestedStep":16}) numberOfPoints,
	VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":1024, "suggestedStep":16}) startPoint,
	VuoInputData(VuoReal, {"default":137.5, "suggestedMin":0.0, "suggestedMax":360.0, "suggestedStep":15}) angle,
	VuoOutputData(VuoList_VuoPoint2d) phyllotaxisPoints)
{
	if (numberOfPoints < 1)
	{
		*phyllotaxisPoints = NULL;
		return;
	}

	*phyllotaxisPoints = VuoListCreateWithCount_VuoPoint2d(numberOfPoints, (VuoPoint2d){ 0, 0 });
	VuoPoint2d *phyllotaxisPointData = VuoListGetData_VuoPoint2d(*phyllotaxisPoints);
	VuoInteger startPointClamped = floor(startPoint - 1);
	VuoInteger offsetPoints = numberOfPoints + startPointClamped;
	for (unsigned int n = startPointClamped; n < offsetPoints; ++n)
	{
		VuoReal a = n * (angle * (M_PI / 180));
		VuoReal r = (radius / sqrt(MAX(1, offsetPoints - 1))) * sqrt(n);
		phyllotaxisPointData[n - startPointClamped] = (VuoPoint2d){r * cos(a) + center.x, r * sin(a) + center.y};
	}
}
