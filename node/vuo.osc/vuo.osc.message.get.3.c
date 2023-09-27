/**
 * @file
 * vuo.osc.message.get.3 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOscMessage.h"

VuoModuleMetadata({
					 "title" : "Get Message Values (3)",
					 "keywords" : [ "address", "data" ],
					 "version" : "1.0.1",
					 "genericTypes": {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType2" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType3" : {
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
		VuoOutputData(VuoGenericType1, {"name":"Value 1"}) data1,
		VuoOutputData(VuoGenericType2, {"name":"Value 2"}) data2,
		VuoOutputData(VuoGenericType3, {"name":"Value 3"}) data3
)
{
	if (!message || !message->dataCount)
		return;

	*address = message->address;

	if (1 <= message->dataCount)
		*data1 = VuoGenericType1_makeFromJson(message->data[0]);

	if (2 <= message->dataCount)
		*data2 = VuoGenericType2_makeFromJson(message->data[1]);

	if (3 <= message->dataCount)
		*data3 = VuoGenericType3_makeFromJson(message->data[2]);
}
