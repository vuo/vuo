/**
 * @file
 * vuo.osc.message.get.11 node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

VuoModuleMetadata({
					 "title" : "Get Message Values",
					 "keywords" : [ "address", "data" ],
					 "version" : "1.0.0",
					 "genericTypes": {
						  "VuoGenericType11" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType12" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType13" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType14" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType15" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType16" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType17" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType18" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType19" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType20" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType21" : {
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
		VuoOutputData(VuoGenericType11) data1,
		VuoOutputData(VuoGenericType12) data2,
		VuoOutputData(VuoGenericType13) data3,
		VuoOutputData(VuoGenericType14) data4,
		VuoOutputData(VuoGenericType15) data5,
		VuoOutputData(VuoGenericType16) data6,
		VuoOutputData(VuoGenericType17) data7,
		VuoOutputData(VuoGenericType18) data8,
		VuoOutputData(VuoGenericType19) data9,
		VuoOutputData(VuoGenericType20) data10,
		VuoOutputData(VuoGenericType21) data11
)
{
	if (!message || !message->data)
		return;

	*address = message->address;

	int dataCount = VuoOscMessage_getDataCount(message);

	if (1 <= dataCount)
		*data1 = VuoGenericType11_makeFromJson(VuoOscMessage_getDataJson(message, 1));

	if (2 <= dataCount)
		*data2 = VuoGenericType12_makeFromJson(VuoOscMessage_getDataJson(message, 2));

	if (3 <= dataCount)
		*data3 = VuoGenericType13_makeFromJson(VuoOscMessage_getDataJson(message, 3));

	if (4 <= dataCount)
		*data4 = VuoGenericType14_makeFromJson(VuoOscMessage_getDataJson(message, 4));

	if (5 <= dataCount)
		*data5 = VuoGenericType15_makeFromJson(VuoOscMessage_getDataJson(message, 5));

	if (6 <= dataCount)
		*data6 = VuoGenericType16_makeFromJson(VuoOscMessage_getDataJson(message, 6));

	if (7 <= dataCount)
		*data7 = VuoGenericType17_makeFromJson(VuoOscMessage_getDataJson(message, 7));

	if (8 <= dataCount)
		*data8 = VuoGenericType18_makeFromJson(VuoOscMessage_getDataJson(message, 8));

	if (9 <= dataCount)
		*data9 = VuoGenericType19_makeFromJson(VuoOscMessage_getDataJson(message, 9));

	if (10 <= dataCount)
		*data10 = VuoGenericType20_makeFromJson(VuoOscMessage_getDataJson(message, 10));

	if (11 <= dataCount)
		*data11 = VuoGenericType21_makeFromJson(VuoOscMessage_getDataJson(message, 11));
}
