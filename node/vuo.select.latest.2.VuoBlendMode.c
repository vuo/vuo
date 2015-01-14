/**
 * @file
 * vuo.select.latest.2.VuoBlendMode node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Latest",
					 "description" :
						"<p>Routes the data from whichever input port received an event to the output port.</p> \
						<p>This node is useful for coalescing data that comes from different data sources. \
						It combines multiple streams of data into a single stream. This is useful when you \
						have multiple nodes that should all send their data into a single input port on another node.</p> \
						<p>When an event comes in through one of the `option` input ports, that port's data and the event \
						are passed on through the output port.</p> \
						<p>When an event comes in through all of the `option` input ports, or through the refresh port and \
						none of the `option` input ports, then the `option1` port's data and the event are passed on through \
						the output port.</p>",
					 "keywords" : [ "coalesce", "join", "combine", "recent", "current",
						"switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBlendMode) option1,
		VuoInputEvent(VuoPortEventBlocking_None,option1) option1Event,
		VuoInputData(VuoBlendMode) option2,
		VuoInputEvent(VuoPortEventBlocking_None,option2) option2Event,
		VuoOutputData(VuoBlendMode) latest
)
{
	if (option2Event && ! option1Event)
		*latest = option2;
	else
		*latest = option1;
}
