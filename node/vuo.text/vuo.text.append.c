/**
 * @file
 * vuo.text.append node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>
#include <string.h>

VuoModuleMetadata({
					 "title" : "Append Texts",
					 "keywords" : [ "concatenate", "strcat", "combine", "join", "merge", "string" ],
					 "version" : "1.1.0",
					 "node": {
						 "exampleCompositions" : [ "CountCharactersInGreetings.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoText) texts,
		VuoInputData(VuoText) separator,
		VuoOutputData(VuoText) compositeText
)
{
	unsigned long textsCount = VuoListGetCount_VuoText(texts);

	// Make room for the separators
	if (textsCount > 0)
		textsCount = textsCount*2 - 1;

	VuoText *textsArray = (VuoText *) malloc(textsCount * sizeof(VuoText));
	for (unsigned long i = 1; i <= textsCount; i += 2)
	{
		textsArray[i-1] = VuoListGetValue_VuoText(texts, i/2 + 1);
		if (i < textsCount)
			textsArray[i] = separator;
	}

	*compositeText = VuoText_append(textsArray, textsCount);

	free(textsArray);
}
