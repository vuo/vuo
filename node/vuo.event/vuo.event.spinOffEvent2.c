/**
 * @file
 * vuo.event.spinOffEvent2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
		VuoInputEvent({"eventBlocking":"none"}) fire,
		VuoOutputTrigger(spunOff, void)
)
{
		spunOff();
}
