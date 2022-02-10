/**
 * @file
 * vuo.type.text.integer node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "json-c/json.h"

VuoModuleMetadata({
	"title": "Convert Text to Integer",
	"keywords": [
		"string",
	],
	"version": "1.0.1",
});

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoOutputData(VuoInteger) integer
)
{
	if (!text)
		return;

	// Since the JSON spec is strict about numbers (e.g., can't have leading "+"),
	// treat text as a JSON string (effectivel putting double-quotes around it),
	// since json-c is more flexible about converting JSON strings to numbers.
	struct json_object *js = json_object_new_string(text);
	*integer = VuoInteger_makeFromJson(js);
	json_object_put(js);
}
