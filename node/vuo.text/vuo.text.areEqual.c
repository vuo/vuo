/**
 * @file
 * vuo.text.areEqual node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <string.h>

VuoModuleMetadata({
					 "title" : "Are Equal",
					 "keywords" : [ "==", "same", "identical", "equivalent", "match", "compare", "strcmp", "string" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoText) texts,
		VuoOutputData(VuoBoolean) equal
)
{
	*equal = true;
	unsigned long textsCount = VuoListGetCount_VuoText(texts);
	for (unsigned long i = 2; i <= textsCount && *equal; ++i)
		// Even though the texts are UTF-8, they're still null-terminated, so we can compare them as C strings.
		if (strcmp(VuoListGetValueAtIndex_VuoText(texts, i - 1), VuoListGetValueAtIndex_VuoText(texts, i)))
			*equal = false;
}
