/**
 * @file
 * vuo.type.boolean.text node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

VuoModuleMetadata({
					 "title" : "Convert Boolean to Text",
					 "keywords" : [ "0", "1", "true", "false" ],
					 "version" : "1.0.1",
					 "node": {
						   "exampleCompositions" : [ ],
						   "isDeprecated": true
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) boolean,
		VuoOutputData(VuoText) text
)
{
	*text = VuoText_makeWithoutCopying(VuoBoolean_getString(boolean));
}
