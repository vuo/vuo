﻿/**
 * @file
 * vuo.text.areEqual node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>

VuoModuleMetadata({
					 "title" : "Are Equal",
					 "keywords" : [ "==", "same", "identical", "equivalent", "match", "compare", "strcmp", "string" ],
					 "version" : "1.0.1",
					 "node": {
						 "isDeprecated": true
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
		if (! VuoText_areEqual(VuoListGetValue_VuoText(texts, i - 1), VuoListGetValue_VuoText(texts, i)))
			*equal = false;
}
