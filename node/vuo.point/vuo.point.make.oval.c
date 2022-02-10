/**
 * @file
 * vuo.point.make.oval node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Make Points along Oval",
	"keywords": [
		"math", "number", "list",
		"shape", "interpolate",
		"arc", "curve", "ellipse",
	],
	"version": "1.0.0",
	"node" : {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoPoint2d, {"default":[0,0], "suggestedMin":[-1,-1], "suggestedMax":[1,1]}) center,
	VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":2}) width,
	VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":2}) height,
	VuoInputData(VuoInteger, {"name":"Number of Points", "default":32, "suggestedMin":0, "suggestedMax":128, "suggestedStep":16}) numberOfPoints,
	VuoInputData(VuoReal, {"default":0,   "suggestedMin":0, "suggestedMax":360, "suggestedStep":15}) startAngle,
	VuoInputData(VuoReal, {"default":180, "suggestedMin":0, "suggestedMax":360, "suggestedStep":15}) endAngle,
	VuoOutputData(VuoList_VuoPoint2d) ovalPoints
)
{
	if (numberOfPoints < 1)
	{
		*ovalPoints = NULL;
		return;
	}

	*ovalPoints               = VuoListCreateWithCount_VuoPoint2d(numberOfPoints, (VuoPoint2d){ 0, 0 });
	VuoPoint2d *ovalPointData = VuoListGetData_VuoPoint2d(*ovalPoints);
	VuoReal spanRadians = (endAngle - startAngle) * M_PI / 180 / MAX(1, numberOfPoints - 1);
	VuoReal startAngleRadians = startAngle * M_PI / 180;
	for (VuoInteger i = 0; i < numberOfPoints; ++i)
		ovalPointData[i] = (VuoPoint2d){
			cos(spanRadians * i + startAngleRadians) * width/2  + center.x,
			sin(spanRadians * i + startAngleRadians) * height/2 + center.y,
		};
}
