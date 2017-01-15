/**
 * @file
 * vuo.test.registerTwice node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Register Twice",
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
	VuoRegister(x, free);  // intentional bug
	VuoRetain(x);
	VuoRelease(x);
	*outEvent = 0;
}
