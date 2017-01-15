/**
 * @file
 * vuo.osc.message.make.1 node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

VuoModuleMetadata({
					 "title" : "Make Message",
					 "keywords" : [ ],
					 "version" : "1.0.0",
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
		VuoInputData(VuoGenericType10) data1,
		VuoInputData(VuoGenericType11) data2,
		VuoInputData(VuoGenericType12) data3,
		VuoInputData(VuoGenericType13) data4,
		VuoInputData(VuoGenericType14) data5,
		VuoInputData(VuoGenericType15) data6,
		VuoInputData(VuoGenericType16) data7,
		VuoInputData(VuoGenericType17) data8,
		VuoInputData(VuoGenericType18) data9,
		VuoInputData(VuoGenericType19) data10,
		VuoInputData(VuoGenericType20) data11,
		VuoOutputData(VuoOscMessage) message
)
{
	if (!address)
		return;

	struct json_object *data = json_object_new_array();
	json_object_array_put_idx(data, 0, VuoGenericType10_getJson(data1));
	json_object_array_put_idx(data, 1, VuoGenericType11_getJson(data2));
	json_object_array_put_idx(data, 2, VuoGenericType12_getJson(data3));
	json_object_array_put_idx(data, 3, VuoGenericType13_getJson(data4));
	json_object_array_put_idx(data, 4, VuoGenericType14_getJson(data5));
	json_object_array_put_idx(data, 5, VuoGenericType15_getJson(data6));
	json_object_array_put_idx(data, 6, VuoGenericType16_getJson(data7));
	json_object_array_put_idx(data, 7, VuoGenericType17_getJson(data8));
	json_object_array_put_idx(data, 8, VuoGenericType18_getJson(data9));
	json_object_array_put_idx(data, 9, VuoGenericType19_getJson(data10));
	json_object_array_put_idx(data, 10, VuoGenericType20_getJson(data11));

	*message = VuoOscMessage_make(address, data);
}
