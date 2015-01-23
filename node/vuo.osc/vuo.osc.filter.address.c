/**
 * @file
 * vuo.osc.filter.address node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

#include <string.h>

VuoModuleMetadata({
					 "title" : "Filter by Address",
					 "keywords" : [ "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "1.0.0",
				 });

void nodeEvent
(
		VuoInputData(VuoOscMessage) message,
		VuoInputEvent(VuoPortEventBlocking_Door, message) messageEvent,

		VuoInputData(VuoText, {"default":"/example"}) address,
		VuoInputEvent(VuoPortEventBlocking_Wall, address) addressEvent,

		VuoOutputData(VuoOscMessage) filteredMessage,
		VuoOutputEvent(filteredMessage) filteredMessageEvent
)
{
	if (strcmp(message->address, address))
		return;

	*filteredMessage = message;
	*filteredMessageEvent = true;
}
