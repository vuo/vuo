/**
 * @file
 * vuo.color.make.rgb node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Make RGB Color",
					 "keywords" : [ "alpha", "transparent", "channel", "tone", "chroma", "rgba", ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) red,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) green,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) blue,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) opacity,
		VuoOutputData(VuoColor) color
)
{
	*color = VuoColor_makeWithRGBA(red, green, blue, opacity);
}
