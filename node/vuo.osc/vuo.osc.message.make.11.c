/**
 * @file
 * vuo.osc.message.make.1 node implementation.
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
					 "title" : "Make Message (11)",
					 "keywords" : [ ],
					 "version" : "1.1.0",
					 "genericTypes": {
						 "VuoGenericType10" : {
							 "defaultType" : "VuoReal",
							 "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						 },
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
						 }
					 },
					 "node": {
						 "exampleCompositions": [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"/example"}) address,
		VuoInputData(VuoGenericType10, {"name":"Value 1"}) data1,
		VuoInputData(VuoOscType, {"default":"auto"}) type1,
		VuoInputData(VuoGenericType11, {"name":"Value 2"}) data2,
		VuoInputData(VuoOscType, {"default":"auto"}) type2,
		VuoInputData(VuoGenericType12, {"name":"Value 3"}) data3,
		VuoInputData(VuoOscType, {"default":"auto"}) type3,
		VuoInputData(VuoGenericType13, {"name":"Value 4"}) data4,
		VuoInputData(VuoOscType, {"default":"auto"}) type4,
		VuoInputData(VuoGenericType14, {"name":"Value 5"}) data5,
		VuoInputData(VuoOscType, {"default":"auto"}) type5,
		VuoInputData(VuoGenericType15, {"name":"Value 6"}) data6,
		VuoInputData(VuoOscType, {"default":"auto"}) type6,
		VuoInputData(VuoGenericType16, {"name":"Value 7"}) data7,
		VuoInputData(VuoOscType, {"default":"auto"}) type7,
		VuoInputData(VuoGenericType17, {"name":"Value 8"}) data8,
		VuoInputData(VuoOscType, {"default":"auto"}) type8,
		VuoInputData(VuoGenericType18, {"name":"Value 9"}) data9,
		VuoInputData(VuoOscType, {"default":"auto"}) type9,
		VuoInputData(VuoGenericType19, {"name":"Value 10"}) data10,
		VuoInputData(VuoOscType, {"default":"auto"}) type10,
		VuoInputData(VuoGenericType20, {"name":"Value 11"}) data11,
		VuoInputData(VuoOscType, {"default":"auto"}) type11,
		VuoOutputData(VuoOscMessage) message
)
{
	if (!address)
		return;

	struct json_object *data[11];
	VuoOscType dataTypes[11];

		 data[0] = VuoGenericType10_getJson(data1);
	dataTypes[0] = type1;

		 data[1] = VuoGenericType11_getJson(data2);
	dataTypes[1] = type2;

		 data[2] = VuoGenericType12_getJson(data3);
	dataTypes[2] = type3;

		 data[3] = VuoGenericType13_getJson(data4);
	dataTypes[3] = type4;

		 data[4] = VuoGenericType14_getJson(data5);
	dataTypes[4] = type5;

		 data[5] = VuoGenericType15_getJson(data6);
	dataTypes[5] = type6;

		 data[6] = VuoGenericType16_getJson(data7);
	dataTypes[6] = type7;

		 data[7] = VuoGenericType17_getJson(data8);
	dataTypes[7] = type8;

		 data[8] = VuoGenericType18_getJson(data9);
	dataTypes[8] = type9;

		 data[9] = VuoGenericType19_getJson(data10);
	dataTypes[9] = type10;

		 data[10] = VuoGenericType20_getJson(data11);
	dataTypes[10] = type11;

	*message = VuoOscMessage_make(address, 11, data, dataTypes);
}
