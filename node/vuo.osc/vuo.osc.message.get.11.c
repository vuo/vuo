/**
 * @file
 * vuo.osc.message.get.11 node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

VuoModuleMetadata({
					 "title" : "Get Message Values (11)",
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
		VuoOutputData(VuoGenericType11, {"name":"Value 1"}) data1,
		VuoOutputData(VuoGenericType12, {"name":"Value 2"}) data2,
		VuoOutputData(VuoGenericType13, {"name":"Value 3"}) data3,
		VuoOutputData(VuoGenericType14, {"name":"Value 4"}) data4,
		VuoOutputData(VuoGenericType15, {"name":"Value 5"}) data5,
		VuoOutputData(VuoGenericType16, {"name":"Value 6"}) data6,
		VuoOutputData(VuoGenericType17, {"name":"Value 7"}) data7,
		VuoOutputData(VuoGenericType18, {"name":"Value 8"}) data8,
		VuoOutputData(VuoGenericType19, {"name":"Value 9"}) data9,
		VuoOutputData(VuoGenericType20, {"name":"Value 10"}) data10,
		VuoOutputData(VuoGenericType21, {"name":"Value 11"}) data11
)
{
	if (!message || !message->dataCount)
		return;

	*address = message->address;

	if (1 <= message->dataCount)
		*data1 = VuoGenericType11_makeFromJson(message->data[0]);

	if (2 <= message->dataCount)
		*data2 = VuoGenericType12_makeFromJson(message->data[1]);

	if (3 <= message->dataCount)
		*data3 = VuoGenericType13_makeFromJson(message->data[2]);

	if (4 <= message->dataCount)
		*data4 = VuoGenericType14_makeFromJson(message->data[3]);

	if (5 <= message->dataCount)
		*data5 = VuoGenericType15_makeFromJson(message->data[4]);

	if (6 <= message->dataCount)
		*data6 = VuoGenericType16_makeFromJson(message->data[5]);

	if (7 <= message->dataCount)
		*data7 = VuoGenericType17_makeFromJson(message->data[6]);

	if (8 <= message->dataCount)
		*data8 = VuoGenericType18_makeFromJson(message->data[7]);

	if (9 <= message->dataCount)
		*data9 = VuoGenericType19_makeFromJson(message->data[8]);

	if (10 <= message->dataCount)
		*data10 = VuoGenericType20_makeFromJson(message->data[9]);

	if (11 <= message->dataCount)
		*data11 = VuoGenericType21_makeFromJson(message->data[10]);
}
