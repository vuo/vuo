/**
 * @file
 * vuo.osc.filter.address node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscMessage.h"

#include <string.h>

VuoModuleMetadata({
					 "title" : "Filter by Address",
					 "keywords" : [ "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "1.0.1",
				 });

void nodeEvent
(
		VuoInputData(VuoOscMessage) message,
		VuoInputEvent({"eventBlocking":"door","data":"message"}) messageEvent,

		VuoInputData(VuoText, {"default":"/example"}) address,
		VuoInputEvent({"eventBlocking":"wall","data":"address"}) addressEvent,

		VuoOutputData(VuoOscMessage) filteredMessage,
		VuoOutputEvent({"data":"filteredMessage"}) filteredMessageEvent
)
{
	if (!message || !message->address || !address || strcmp(message->address, address))
		return;

	*filteredMessage = message;
	*filteredMessageEvent = true;
}
