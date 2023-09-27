/**
 * @file
 * vuo.color.make.hsl node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Make HSL Color",
					 "keywords" : [
						 "alpha", "transparent",
						 "channel", "tone", "chroma",
						 "brightness", "luminance", "luma", "hsla",
					 ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "ChangeSaturationAndLightness.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) hue,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) saturation,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) lightness,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) opacity,
		VuoOutputData(VuoColor) color
)
{
	*color = VuoColor_makeWithHSLA(hue, saturation, lightness, opacity);
}
