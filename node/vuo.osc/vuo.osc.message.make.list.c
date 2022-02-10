/**
 * @file
 * vuo.osc.message.make.list node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

VuoModuleMetadata({
	"title": "Make Message (List)",
	"keywords": [ ],
	"version": "1.1.0",
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": "VuoReal",
			"compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ],
		}
	},
	"node": {
		"exampleCompositions": [ ],
	}
});

void nodeEvent
(
	VuoInputData(VuoText, {"default":"/example"}) address,
	VuoInputData(VuoList_VuoGenericType1) values,
	VuoInputData(VuoOscType, {"default":"auto"}) type,
	VuoOutputData(VuoOscMessage) message
)
{
	if (!address || !values)
		return;

	unsigned long count = VuoListGetCount_VuoGenericType1(values);
	struct json_object *jsdata[count];
	VuoOscType dataTypes[count];

	for (unsigned long i = 0; i < count; ++i)
	{
		jsdata[i] = VuoGenericType1_getJson(VuoListGetValue_VuoGenericType1(values, i+1));
		dataTypes[i] = type;
	}
	*message = VuoOscMessage_make(address, count, jsdata, dataTypes);
}
