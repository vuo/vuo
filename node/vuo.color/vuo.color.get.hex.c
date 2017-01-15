/**
 * @file
 * vuo.color.get.hex node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Hex Color Value",
					 "keywords" : [ "red", "green", "blue", "alpha", "transparent", "channel", "tone", "chroma",
						 "hexadecimal", "CSS", "HTML"
					 ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "PickColor.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoInputData(VuoBoolean, {"default":false}) includeOpacity,
		VuoOutputData(VuoText) hex
)
{
	*hex = VuoColor_getHex(color, includeOpacity);
}
