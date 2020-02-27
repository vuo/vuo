/**
 * @file
 * vuo.event.fireOnStart node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Fire on Live-Editing Reload",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node" : {
						 "isDeprecated": true,  // Not really deprecated, but for internal use only. Don't show in node library.
						 "exampleCompositions" : [ ]
					 }
				 });


bool * nodeInstanceInit(void)
{
	return NULL;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(bool *) instance,
		VuoOutputTrigger(reloaded,void)
)
{
	reloaded();
}

void nodeInstanceEvent
(
		VuoInstanceData(bool *) instance
)
{
}
