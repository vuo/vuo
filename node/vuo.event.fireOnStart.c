/**
 * @file
 * vuo.event.fireOnStart node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Fire on Start",
					 "description" :
						"<p>Fires an event when the composition starts.</p> \
						<p>Typically you would want just one of these nodes in a composition. \
						Multiple `Fire on Start` nodes fire separate events and are not synchronized with each other.</p> \
						<p>There is no guarantee that this node will be the first to fire within a composition. \
						For example, a rapidly firing `Fire Periodically` node may fire first.</p>",
					 "keywords" : [ "begin", "init", "launch", "main", "pulse", "signal" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


bool * nodeInstanceInit(void)
{
	bool *hasStarted = (bool *) malloc(sizeof(bool));
	VuoRegister(hasStarted, free);
	*hasStarted = false;
	return hasStarted;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(bool *) hasStarted,
		VuoOutputTrigger(started,void)
)
{
	if (! **hasStarted)
	{
		started();
		**hasStarted = true;
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(bool *) hasStarted
)
{
}

void nodeInstanceFini
(
		VuoInstanceData(bool *) hasStarted
)
{
}
