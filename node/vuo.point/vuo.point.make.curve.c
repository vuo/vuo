/**
 * @file
 * vuo.point.make.curve node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Make Points along Line",
					  "keywords" : [ "math", "number", "shape", "ease", "easing", "linear", "quadratic", "cubic", "circular", "exponential", "logarithmic", "interpolate", "list" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ "DrawPointsAlongCurve.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoGenericType1) startPosition,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":1., "VuoPoint2d":{"x":1.,"y":1.}, "VuoPoint3d":{"x":1.,"y":1.,"z":0.}}}) endPosition,
		VuoInputData(VuoCurve, {"default":"linear"}) curve,
		VuoInputData(VuoCurveEasing, {"default":"in"}) easing,
		VuoInputData(VuoInteger, {"name":"Number of Points", "default":32}) numberOfPoints,
		VuoOutputData(VuoList_VuoGenericType1) points
)
{
	*points = VuoListCreate_VuoGenericType1();
	for (VuoInteger i=0; i<numberOfPoints; ++i)
	{
		VuoGenericType1 point = VuoGenericType1_curve(i, startPosition, endPosition, numberOfPoints-1, curve, easing, VuoLoopType_None);
		VuoListAppendValue_VuoGenericType1(*points, point);
	}
}
