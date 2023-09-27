/**
 * @file
 * vuo.test.spinOffWithCount node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Spin Off with Count",
					 "version" : "1.0.0",
				 });

VuoInteger * nodeInstanceInit(void)
{
	VuoInteger *count = (VuoInteger *)malloc(sizeof(VuoInteger));
	VuoRegister(count, free);
	*count = 0;
	return count;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoInteger *) count,
		VuoOutputTrigger(spunOff,VuoInteger)
)
{
	**count += 1;
	spunOff(**count);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoInteger *) count
)
{
}
