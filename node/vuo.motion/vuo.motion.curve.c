/**
 *@file
 * vuo.math.curve node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Curve",
					  "keywords" : [ "ease", "easing", "quadratic", "gravity", "bounce", "cubic", "circular", "exponential", "logarithmic", "interpolation",
						  /* Samia Halaby */ "yoyo", "yo-yo",
						  /* QC */ "LFO", "Wave Generator"
					  ],
					  "version" : "2.0.1",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "CompareEasingCurves.vuo", "DrawCurve.vuo", "ExplodeClay.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoReal) time,
		VuoInputData(VuoGenericType1) startPosition,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":1., "VuoPoint2d":{"x":1.,"y":1.}, "VuoPoint3d":{"x":1.,"y":1.,"z":1.}}}) endPosition,
		VuoInputData(VuoReal, {"default":1.}) duration,
		VuoInputData(VuoCurve, {"default":"quadratic"}) curve,
		VuoInputData(VuoCurveEasing, {"default":"in"}) easing,
		VuoInputData(VuoLoopType, {"default":"none"}) loop,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1}) phase,
		VuoOutputData(VuoGenericType1) value
)
{
	VuoReal timeAdjusted = time;
	if (loop == VuoLoopType_Mirror)
		timeAdjusted += phase * duration * 2;
	else
		timeAdjusted += phase * duration;

	*value = VuoGenericType1_curve(timeAdjusted, startPosition, endPosition, duration, curve, easing, loop);
}
