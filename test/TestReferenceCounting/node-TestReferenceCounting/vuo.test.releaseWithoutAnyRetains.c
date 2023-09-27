/**
 * @file
 * vuo.test.releaseWithoutAnyRetains node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Release without Any Retains",
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
	VuoRelease(x);  // intentional bug
	VuoRetain(x);
	VuoRelease(x);
	*outEvent = 0;
}
