/**
 * @file
 * vuo.point.make.spline node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Points along Spline",
					  "keywords" : [
						  "math", "number", "shape", "interpolate", "curve",
						  "linear",
						  "cubic", "hermite",
						  "Catmull-Rom", // T=C=B=0
						  "cardinal", // variable T, C=B=0
						  "TCB", "Kochanek–Bartels",
						  "keyframe",
						  "list",
					  ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ "DrawPointsAlongSpline.vuo" ]
					  }
				 });

static double tcb(double p0, double p1, double p2, double p3, double t, double tension, double continuity, double bias)
{
	float h1 =  2.*t*t*t - 3.*t*t + 1.; // basis function 1
	float h2 = -2.*t*t*t + 3.*t*t;      // basis function 2
	float h3 =     t*t*t - 2.*t*t + t ; // basis function 3
	float h4 =     t*t*t -    t*t;      // basis function 4
	float ds = (1.-tension) * (1.+continuity) * (1.+bias) * (p1-p0) / 2.
			 + (1.-tension) * (1.-continuity) * (1.-bias) * (p2-p1) / 2.;
	float dd = (1.-tension) * (1.-continuity) * (1.+bias) * (p2-p1) / 2.
			 + (1.-tension) * (1.+continuity) * (1.-bias) * (p3-p2) / 2.;
	return h1*p1
		 + h2*p2
		 + h3*ds
		 + h4*dd;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

static VuoReal spline_VuoReal(VuoReal *inputPointData, long point, long inputPointCount, double t, double tension, double continuity, double bias)
{
	return tcb(inputPointData[MAX(0,point-1)], inputPointData[point], inputPointData[point+1], inputPointData[MIN(inputPointCount-1,point+2)], t, tension, continuity, bias);
}

static VuoPoint2d spline_VuoPoint2d(VuoPoint2d *inputPointData, long point, long inputPointCount, double t, double tension, double continuity, double bias)
{
	return (VuoPoint2d){
		tcb(inputPointData[MAX(0,point-1)].x, inputPointData[point].x, inputPointData[point+1].x, inputPointData[MIN(inputPointCount-1,point+2)].x, t, tension, continuity, bias),
		tcb(inputPointData[MAX(0,point-1)].y, inputPointData[point].y, inputPointData[point+1].y, inputPointData[MIN(inputPointCount-1,point+2)].y, t, tension, continuity, bias)
	};
}

static VuoPoint3d spline_VuoPoint3d(VuoPoint3d *inputPointData, long point, long inputPointCount, double t, double tension, double continuity, double bias)
{
	return (VuoPoint3d){
		tcb(inputPointData[MAX(0,point-1)].x, inputPointData[point].x, inputPointData[point+1].x, inputPointData[MIN(inputPointCount-1,point+2)].x, t, tension, continuity, bias),
		tcb(inputPointData[MAX(0,point-1)].y, inputPointData[point].y, inputPointData[point+1].y, inputPointData[MIN(inputPointCount-1,point+2)].y, t, tension, continuity, bias),
		tcb(inputPointData[MAX(0,point-1)].z, inputPointData[point].z, inputPointData[point+1].z, inputPointData[MIN(inputPointCount-1,point+2)].z, t, tension, continuity, bias)
	};
}

#pragma clang diagnostic pop

void nodeEvent
(
	VuoInputData(VuoList_VuoGenericType1) controlPoints,
	VuoInputData(VuoInteger, {"default":16, "suggestedMin":1, "suggestedMax":64}) tweenPointCount,
	VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-1.0, "suggestedMax":1.0, "suggestedStep":0.1}) tension,
	VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-1.0, "suggestedMax":1.0, "suggestedStep":0.1}) continuity,
	VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-1.0, "suggestedMax":1.0, "suggestedStep":0.1}) bias,
	VuoOutputData(VuoList_VuoGenericType1) splinePoints
)
{
	unsigned long inputPointCount = VuoListGetCount_VuoGenericType1(controlPoints);
	if (inputPointCount == 0)
	{
		*splinePoints = NULL;
		return;
	}

	VuoGenericType1 *inputPointData = VuoListGetData_VuoGenericType1(controlPoints);

	unsigned long tweenPointCountClamped = MAX(1,tweenPointCount);
	unsigned long splineCount = (inputPointCount - 1) * tweenPointCountClamped + 1;
	*splinePoints = VuoListCreateWithCount_VuoGenericType1(splineCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *splineData = VuoListGetData_VuoGenericType1(*splinePoints);
	for (unsigned long point = 0; point < inputPointCount - 1; ++point)
	{
		splineData[point*tweenPointCountClamped] = inputPointData[point];

		for (unsigned long splinePoint = 1; splinePoint < tweenPointCountClamped; ++splinePoint)
			splineData[point*tweenPointCountClamped + splinePoint] = spline_VuoGenericType1(
						inputPointData, point, inputPointCount,
						(float)splinePoint / tweenPointCount,
						tension, continuity, bias);
	}

	splineData[splineCount - 1] = inputPointData[inputPointCount - 1];
}
