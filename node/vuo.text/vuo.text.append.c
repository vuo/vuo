/**
 * @file
 * vuo.text.append node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>
#include <string.h>

VuoModuleMetadata({
					 "title" : "Append Texts",
					 "keywords" : [ "concatenate", "strcat", "combine", "join", "merge", "string" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "RevealWord.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoText) texts,
		VuoOutputData(VuoText) compositeText
)
{
	unsigned long textsCount = VuoListGetCount_VuoText(texts);

	VuoText *textsArray = (VuoText *) malloc(textsCount * sizeof(VuoText));
	for (unsigned long i = 1; i <= textsCount; ++i)
		textsArray[i-1] = VuoListGetValue_VuoText(texts, i);

	*compositeText = VuoText_append(textsArray, textsCount);

	free(textsArray);
}
