/**
 * @file
 * vuo.color.palette.make.gradient node implementation.
 *
 * @copyright Copyright © 2012–2024 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Make Gradient Palette",
	"keywords": [
		"ramp", "interpolate", "quantize", "linear",
		"generate", "create",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ "vuo-example://vuo.scene/DisplayGradientOfSpheres.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) anchorColors,
	VuoInputData(VuoInteger, {"default":8, "suggestedMin":1, "suggestedMax":256}) tweenColorCount,
	VuoOutputData(VuoList_VuoColor) palette)
{
	unsigned long anchorColorCount = VuoListGetCount_VuoColor(anchorColors);

	if (anchorColorCount == 0)
	{
		*palette = VuoListCreate_VuoColor();
		return;
	}

	VuoColor *anchorColorData = VuoListGetData_VuoColor(anchorColors);

	unsigned long tweenColorCountClamped = MAX(0, tweenColorCount);

	unsigned long paletteColorCount = anchorColorCount + (anchorColorCount - 1) * tweenColorCountClamped;
	*palette = VuoListCreateWithCount_VuoColor(paletteColorCount, (VuoColor){0,0,0,1});
	VuoColor *paletteData = VuoListGetData_VuoColor(*palette);

	for (unsigned long anchorInInput = 0; anchorInInput < anchorColorCount; ++anchorInInput)
	{
		unsigned long anchorInOutput = anchorInInput * (tweenColorCountClamped + 1);
		paletteData[anchorInOutput] = anchorColorData[anchorInInput];

		if (anchorInInput < anchorColorCount - 1)
			for (unsigned long tween = 1; tween <= tweenColorCountClamped; ++tween)
				paletteData[anchorInOutput + tween] = VuoColor_lerp(anchorColorData[anchorInInput], anchorColorData[anchorInInput + 1],
																	tween/(tweenColorCountClamped + 1.f));
	}
}
