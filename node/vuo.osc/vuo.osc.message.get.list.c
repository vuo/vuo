/**
 * @file
 * vuo.osc.message.get.list node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

VuoModuleMetadata({
	"title": "Get Message Values (List)",
	"keywords": [ "address", "data" ],
	"version": "1.0.0",
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
	VuoInputData(VuoOscMessage) message,
	VuoOutputData(VuoText) address,
	VuoOutputData(VuoList_VuoGenericType1) values
)
{
	*values = VuoListCreate_VuoGenericType1();
	if (!message)
		return;

	*address = message->address;

	for (int i = 0; i < message->dataCount; ++i)
		VuoListAppendValue_VuoGenericType1(*values, VuoGenericType1_makeFromJson(message->data[i]));
}
