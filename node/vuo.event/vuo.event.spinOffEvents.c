/**
 * @file
 * vuo.event.spinOffEvents node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Spin Off Events",
					 "keywords" : [ "scatter", "fork", "spawn", "thread", "multithread", "multicore", "parallel", "concurrent", "asynchronous", "background",
						 "iterate", "repeat", "multiple", "many", "foreach", "each", "loop", "while", "cycle" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "SpinPsychedelicCheckerboard.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":2}) fire,
		VuoInputEvent({"data":"fire", "eventBlocking":"none"}) fireEvent,
		VuoOutputTrigger(spunOffIndex, VuoInteger)
)
{
	for (VuoInteger i = 0; i < fire; ++i)
		spunOffIndex(i + 1);
}
