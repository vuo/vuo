/**
 * @file
 * vuo.type.list.text.real node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "json-c/json.h"

VuoModuleMetadata({
					  "title": "Convert Text List to Real List",
					  "description": "Outputs a list containing real numbers representing the input list's text numbers.  See `Convert Text to Real` for info on how the text is interpreted.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoText) texts,
	VuoOutputData(VuoList_VuoReal) reals
)
{
	unsigned long count = VuoListGetCount_VuoText(texts);
	*reals = VuoListCreateWithCount_VuoReal(count, 0);
	VuoText *textsArray = VuoListGetData_VuoText(texts);
	VuoReal *realsArray = VuoListGetData_VuoReal(*reals);
	for (unsigned long i = 0; i < count; ++i)
		if (textsArray[i])
		{
			// Since the JSON spec is strict about numbers (e.g., can't have leading "+" or "."),
			// treat text as a JSON string (effectivel putting double-quotes around it),
			// since json-c is more flexible about converting JSON strings to numbers.
			struct json_object *js = json_object_new_string(textsArray[i]);
			realsArray[i] = VuoReal_makeFromJson(js);
			json_object_put(js);
		}
}
