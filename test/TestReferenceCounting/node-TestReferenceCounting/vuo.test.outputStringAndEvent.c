/**
 * @file
 * vuo.test.outputStringAndEvent node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Output String and Event",
					 "description" : "",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoText) outString,
		VuoOutputData(VuoInteger) outEvent
)
{
	*outString = VuoText_make("initializer string");
	*outEvent = 0;
}
