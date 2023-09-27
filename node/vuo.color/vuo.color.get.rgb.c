/**
 * @file
 * vuo.color.get.rgb node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Get RGB Color Values",
					 "keywords" : [ "alpha", "transparent", "channel", "tone", "chroma", "rgba", ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoOutputData(VuoReal) red,
		VuoOutputData(VuoReal) green,
		VuoOutputData(VuoReal) blue,
		VuoOutputData(VuoReal) opacity
)
{
	VuoColor_getRGBA(color, red, green, blue, opacity);
}
