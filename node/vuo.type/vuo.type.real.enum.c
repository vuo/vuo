/**
 * @file
 * vuo.type.real.enum node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Convert Real to Options",
	"keywords" : [ ],
	"version" : "1.0.0",
	"genericTypes" : {
		"VuoGenericType1" : {
			"compatibleTypes" : [
				"VuoAnchor",
				"VuoAudioBinAverageType",
				"VuoAudioBins",
				"VuoAudioEncoding",
				"VuoBaudRate",
				"VuoBlackmagicConnection",
				"VuoBlackmagicVideoMode",
				"VuoBlendMode",
				"VuoBlurShape",
				"VuoBoolean",
				"VuoColorSample",
				"VuoControlCode",
				"VuoCoordinateUnit",
				"VuoCursor",
				"VuoCurve",
				"VuoCurveEasing",
				"VuoDeinterlacing",
				"VuoDiode",
				"VuoDispersion",
				"VuoDisplacement",
				"VuoDistribution3d",
				"VuoDmxColorMap",
				"VuoDurationType",
				"VuoExtrapolationMode",
				"VuoFileType",
				"VuoGradientNoise",
				"VuoGridType",
				"VuoHorizontalAlignment",
				"VuoHorizontalReflection",
				"VuoHorizontalSide",
				"VuoImageColorDepth",
				"VuoImageFormat",
				"VuoImageNoise",
				"VuoImageStereoType",
				"VuoImageWrapMode",
				"VuoKey",
				"VuoLeapPointableType",
				"VuoLeapTouchZone",
				"VuoListPosition",
				"VuoLoopType",
				"VuoModifierKey",
				"VuoMouseButton",
				"VuoMovieImageEncoding",
				"VuoMultisample",
				"VuoNoise",
				"VuoNotePriority",
				"VuoNumberComparison",
				"VuoNumberFormat",
				"VuoOrientation",
				"VuoOscType",
				"VuoParity",
				"VuoPixelShape",
				"VuoProjectionType",
				"VuoRoundingMethod",
				"VuoSceneObjectType",
				"VuoSizingMode",
				"VuoSortOrder",
//				"VuoSpeechVoice", @todo figure out how to make reference-counting work
				"VuoTableFormat",
				"VuoTempoRange",
				"VuoTextCase",
//				"VuoTextComparison",
				"VuoTextSort",
				"VuoThresholdType",
				"VuoTimeFormat",
				"VuoTimeUnit",
				"VuoVertexAttribute",
				"VuoVerticalAlignment",
				"VuoVerticalReflection",
				"VuoVideoOptimization",
				"VuoWave",
				"VuoWeekday",
				"VuoWrapMode",
			],
		},
	},
});

void nodeEvent
(
	VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0.0,"suggestedMax":1.0}) real,
	VuoOutputData(VuoGenericType1, {"name":"Option"}) value
)
{
	VuoList_VuoGenericType1 allowedValues = VuoGenericType1_getAllowedValues();
	VuoLocal(allowedValues);
	VuoInteger allowedValueCount = VuoListGetCount_VuoGenericType1(allowedValues);
	VuoInteger index = VuoInteger_clamp(lround(real * allowedValueCount + .5), 1, allowedValueCount);
	*value = VuoListGetValue_VuoGenericType1(allowedValues, index);
}
