/**
 * @file
 * vuo.test.retainWithoutRelease node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Retain without Release",
					 "description" : "This node class contains an intentional bug for testing.",
					 "version" : "1.0.0",
				 });

void nodeEvent
(
		VuoOutputData(VuoInteger) outEvent
)
{
	char *x = malloc(1);
	VuoRegister(x, free);
	VuoRetain(x);  // intentional bug
	*outEvent = 0;
}
