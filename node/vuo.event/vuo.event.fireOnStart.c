﻿/**
 * @file
 * vuo.event.fireOnStart node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Fire on Start",
					 "keywords" : [ "begin", "init", "launch", "first", "main", "pulse", "signal" ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ ]  // no examples needed
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
