/**
 * @file
 * vuo.test.details node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Port Details",
					 "description" : "",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"test":"in1 data"}) in1,
		VuoInputEvent({"data":"in1","test":"in1 event"}) in1Event,
		VuoInputEvent({"test":"in2 event"}) in2,
		VuoOutputData(VuoText, {"test":"out1 data"}) out1,
		VuoOutputEvent({"data":"out1","test":"out1 event"}) out1Event,
		VuoOutputEvent({"test":"out2 event"}) out2,
		VuoOutputTrigger(trig1, VuoText, {"test":"trig1 trigger"}),
		VuoOutputTrigger(trig2, VuoText, {"test":"trig2 trigger"}),
		VuoOutputTrigger(trig3, void, {"test":"trig3 trigger"})
)
{
}
