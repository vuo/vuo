/**
 * @file
 * vuo.osc.message.get.4 node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

VuoModuleMetadata({
					 "title" : "Get Message Values",
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
						  },
						  "VuoGenericType4" : {
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
		VuoOutputData(VuoGenericType1) data1,
		VuoOutputData(VuoGenericType2) data2,
		VuoOutputData(VuoGenericType3) data3,
		VuoOutputData(VuoGenericType4) data4
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

	if (4 <= message->dataCount)
		*data4 = VuoGenericType4_makeFromJson(message->data[3]);
}
