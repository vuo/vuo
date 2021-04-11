/**
 * @file
 * vuo.osc.message.get.1 node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

VuoModuleMetadata({
					 "title" : "Get Message Values (1)",
					 "keywords" : [ "address", "data" ],
					 "version" : "1.0.1",
					 "genericTypes": {
						 "VuoGenericType1" : {
							 "defaultType" : "VuoReal",
							 "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						 }
					 },
					 "node": {
						 "exampleCompositions": [ "ReceiveOsc.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoOscMessage) message,
		VuoOutputData(VuoText) address,
		VuoOutputData(VuoGenericType1, {"name":"Value 1"}) data1
)
{
	if (!message || !message->dataCount)
		return;

	*address = message->address;

	if (1 <= message->dataCount)
		*data1 = VuoGenericType1_makeFromJson(message->data[0]);
}
