/**
 * @file
 * vuo.type.boolean.text node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

VuoModuleMetadata({
					 "title" : "Convert Boolean to Text",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) boolean,
		VuoOutputData(VuoText) text
)
{
	char *textAsCString = VuoBoolean_stringFromValue(boolean);
	*text = VuoText_make(textAsCString);
	free(textAsCString);
}
