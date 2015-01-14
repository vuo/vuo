/**
 * @file
 * vuo.event.spinOffEvent node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Spin Off Event",
					 "description" :
						"<p>When this node receives an event, it fires a separate event. \
						<p>This node is useful for performing tasks asynchronously (in the background). \
						This allows one part of a composition to do time-consuming work without blocking or slowing down other parts of the composition.</p>",
					 "keywords" : [ "scatter", "fork", "spawn", "thread", "multithread", "multicore", "parallel", "concurrent", "asynchronous", "background" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
 VuoOutputTrigger(spunOff,void)
)
{
	spunOff();
}
