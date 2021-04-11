/**
 * @file
 * vuo.osc.message.make.4 node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
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
					 "title" : "Make Message (4)",
					 "keywords" : [ ],
					 "version" : "1.1.0",
					 "genericTypes": {
						 "VuoGenericType0" : {
							 "defaultType" : "VuoReal",
							 "compatibleTypes": [ "VuoBoolean", "VuoInteger", "VuoReal", "VuoText" ]
						 },
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
						 "exampleCompositions": [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"/example"}) address,
		VuoInputData(VuoGenericType0, {"name":"Value 1"}) data1,
		VuoInputData(VuoOscType, {"default":"auto"}) type1,
		VuoInputData(VuoGenericType1, {"name":"Value 2"}) data2,
		VuoInputData(VuoOscType, {"default":"auto"}) type2,
		VuoInputData(VuoGenericType2, {"name":"Value 3"}) data3,
		VuoInputData(VuoOscType, {"default":"auto"}) type3,
		VuoInputData(VuoGenericType3, {"name":"Value 4"}) data4,
		VuoInputData(VuoOscType, {"default":"auto"}) type4,
		VuoOutputData(VuoOscMessage) message
)
{
	if (!address)
		return;

	struct json_object *data[4];
	VuoOscType dataTypes[4];

		 data[0] = VuoGenericType0_getJson(data1);
	dataTypes[0] = type1;

		 data[1] = VuoGenericType1_getJson(data2);
	dataTypes[1] = type2;

		 data[2] = VuoGenericType2_getJson(data3);
	dataTypes[2] = type3;

		 data[3] = VuoGenericType3_getJson(data4);
	dataTypes[3] = type4;

	*message = VuoOscMessage_make(address, 4, data, dataTypes);
}
