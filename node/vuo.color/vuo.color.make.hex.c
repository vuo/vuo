/**
 * @file
 * vuo.color.make.hex node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <json-c/json.h>

VuoModuleMetadata({
					 "title" : "Make Hex Color",
					 "keywords" : [ "red", "green", "blue", "opacity", "alpha", "transparent", "channel", "tone", "chroma",
						 "hexadecimal", "CSS", "HTML"
					 ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "PickColor.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"#445566"}) hexCode,
		VuoOutputData(VuoColor) color
)
{
	if (!hexCode)
		return;

	VuoText trimmedHexCode = VuoText_trim(hexCode);
	VuoRetain(trimmedHexCode);
	json_object *js = json_object_new_string(trimmedHexCode);
	*color = VuoColor_makeFromJson(js);
	json_object_put(js);
	VuoRelease(trimmedHexCode);
}
