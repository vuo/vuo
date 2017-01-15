/**
 * @file
 * vuo.test.conductor node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

VuoModuleMetadata({
					 "title" : "Conductor",
					 "description" : "Pauses for a random amount of time, then passes `triggerInfoIn` to `triggerInfoOut` and `triggerInfoIn nodeTitle` to `nodeInfo`.",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) triggerInfoIn0,
		VuoInputData(VuoText, {"default":""}) triggerInfoIn1,
		VuoInputData(VuoText, {"default":""}) triggerInfoIn2,
		VuoInputData(VuoText, {"default":""}) triggerInfoIn3,
		VuoInputEvent({"eventBlocking":"none","data":"triggerInfoIn0"}) triggerInfoEvent0,
		VuoInputEvent({"eventBlocking":"none","data":"triggerInfoIn1"}) triggerInfoEvent1,
		VuoInputEvent({"eventBlocking":"none","data":"triggerInfoIn2"}) triggerInfoEvent2,
		VuoInputEvent({"eventBlocking":"none","data":"triggerInfoIn3"}) triggerInfoEvent3,
		VuoInputData(VuoText, {"default":""}) nodeTitle,
		VuoOutputData(VuoText) triggerInfoOut,
		VuoOutputData(VuoText) nodeInfo
)
{
	usleep(arc4random() % 500);  // up to 0.5 milliseconds

	if (triggerInfoEvent3)
		*triggerInfoOut = triggerInfoIn3;
	else if (triggerInfoEvent2)
		*triggerInfoOut = triggerInfoIn2;
	else if (triggerInfoEvent1)
		*triggerInfoOut = triggerInfoIn1;
	else
		*triggerInfoOut = triggerInfoIn0;

	char *ni = (char *)malloc(strlen(*triggerInfoOut) + strlen(nodeTitle) + 2);
	sprintf(ni, "%s %s", *triggerInfoOut, nodeTitle);
	*nodeInfo = VuoText_make(ni);
	free(ni);
}
