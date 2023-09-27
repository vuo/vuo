/**
 * @file
 * vuo.osc.message.get.11 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOscMessage.h"

VuoModuleMetadata({
					 "title" : "Get Message Values (11)",
					 "keywords" : [ "address", "data" ],
					 "version" : "1.0.0",
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
						  },
						  "VuoGenericType5" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType6" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType7" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType8" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType9" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType10" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						  },
						  "VuoGenericType11" : {
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
		VuoOutputData(VuoGenericType3, {"name":"Value 3"}) data3,
		VuoOutputData(VuoGenericType4, {"name":"Value 4"}) data4,
		VuoOutputData(VuoGenericType5, {"name":"Value 5"}) data5,
		VuoOutputData(VuoGenericType6, {"name":"Value 6"}) data6,
		VuoOutputData(VuoGenericType7, {"name":"Value 7"}) data7,
		VuoOutputData(VuoGenericType8, {"name":"Value 8"}) data8,
		VuoOutputData(VuoGenericType9, {"name":"Value 9"}) data9,
		VuoOutputData(VuoGenericType10, {"name":"Value 10"}) data10,
		VuoOutputData(VuoGenericType11, {"name":"Value 11"}) data11
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

	if (5 <= message->dataCount)
		*data5 = VuoGenericType5_makeFromJson(message->data[4]);

	if (6 <= message->dataCount)
		*data6 = VuoGenericType6_makeFromJson(message->data[5]);

	if (7 <= message->dataCount)
		*data7 = VuoGenericType7_makeFromJson(message->data[6]);

	if (8 <= message->dataCount)
		*data8 = VuoGenericType8_makeFromJson(message->data[7]);

	if (9 <= message->dataCount)
		*data9 = VuoGenericType9_makeFromJson(message->data[8]);

	if (10 <= message->dataCount)
		*data10 = VuoGenericType10_makeFromJson(message->data[9]);

	if (11 <= message->dataCount)
		*data11 = VuoGenericType11_makeFromJson(message->data[10]);
}
