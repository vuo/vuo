/**
 * @file
 * vuo.test.triggerCarryingInteger node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Trigger carrying integer",
					 "description" : "",
					 "version" : "1.0.0",
				 });

void * nodeInstanceInit(void)
{
	return 0;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(void *) context,
		VuoOutputTrigger(fired,VuoInteger)
)
{
	fired(-999);
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
