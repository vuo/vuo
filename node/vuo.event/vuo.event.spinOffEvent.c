/**
 * @file
 * vuo.event.spinOffEvent node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Spin Off Event",
					 "keywords" : [ "scatter", "fork", "spawn", "thread", "multithread", "multicore", "parallel", "concurrent", "asynchronous", "background" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "LoadImageAsynchronously.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoOutputTrigger(spunOff,void)
)
{
	spunOff();
}
