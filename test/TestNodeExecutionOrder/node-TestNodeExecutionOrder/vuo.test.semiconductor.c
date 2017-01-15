/**
 * @file
 * vuo.test.semiconductor node implementation.
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
					 "title" : "Semiconductor",
					 "description" : "If the conductive input receives an event, this node pauses for a random amount of time, then passes `triggerInfoConductive` to `triggerInfoOut` and `triggerInfoIn nodeTitle` to `nodeInfo`. Otherwise, it does nothing.",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) triggerInfoConductive,
		VuoInputEvent({"eventBlocking":"none","data":"triggerInfoConductive"}) triggerInfoConductiveEvent,
		VuoInputData(VuoText, {"default":""}) triggerInfoNonConductive,
		VuoInputEvent({"eventBlocking":"wall","data":"triggerInfoNonConductive"}) triggerInfoNonConductiveEvent,
		VuoInputData(VuoText, {"default":""}) nodeTitle,
		VuoOutputData(VuoText) triggerInfoOut,
		VuoOutputEvent({"data":"triggerInfoOut"}) triggerInfoOutEvent,
		VuoOutputData(VuoText) nodeInfo,
		VuoOutputEvent({"data":"nodeInfo"}) nodeInfoEvent
)
{
	if (! triggerInfoConductiveEvent)
		return;

	usleep(arc4random() % 500);  // up to 0.5 milliseconds

	*triggerInfoOut = triggerInfoConductive;

	char *ni = (char *)malloc(strlen(*triggerInfoOut) + strlen(nodeTitle) + 2);
	sprintf(ni, "%s %s", *triggerInfoOut, nodeTitle);
	*nodeInfo = VuoText_make(ni);
	free(ni);

	*triggerInfoOutEvent = true;
	*nodeInfoEvent = true;
}
