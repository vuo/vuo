/**
 * @file
 * vuo.test.triggerCarryingPoint4d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Trigger carrying point4d",
					 "description" : "",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void * nodeInstanceInit(void)
{
	return 0;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(void *) context,
		VuoOutputTrigger(fired,VuoPoint4d)
)
{
	VuoPoint4d p = { -42, 42, -1, 1 };
	fired(p);
}

void nodeInstanceEvent
(
		VuoInstanceData(void *) context
)
{
}

void nodeInstanceFini
(
		VuoInstanceData(void *) context
)
{
}
