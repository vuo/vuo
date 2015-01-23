/**
 * @file
 * vuo.osc.message.get.1 node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
						 "VuoGenericType1" : {
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
		VuoOutputData(VuoGenericType1) data1
)
{
	*address = message->address;

	int dataCount = VuoOscMessage_getDataCount(message);

	if (1 <= dataCount)
		*data1 = VuoGenericType1_valueFromJson(VuoOscMessage_getDataJson(message, 1));
}
