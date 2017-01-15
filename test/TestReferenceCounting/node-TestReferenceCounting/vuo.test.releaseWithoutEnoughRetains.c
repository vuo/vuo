/**
 * @file
 * vuo.test.releaseWithoutEnoughRetains node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Release without Enough Retains",
					 "description" : "This node class contains an intentional bug for testing.",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoInteger) outEvent
)
{
	char *x = malloc(1);
	VuoRegister(x, free);
	VuoRetain(x);
	VuoRelease(x);
	VuoRelease(x);  // intentional bug
	*outEvent = 0;
}
