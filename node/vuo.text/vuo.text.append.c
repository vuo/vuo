/**
 * @file
 * vuo.text.append node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>
#include <string.h>

VuoModuleMetadata({
					 "title" : "Append Texts",
					 "keywords" : [ "concatenate", "strcat", "combine", "join", "merge", "string" ],
					 "version" : "1.2.0",
					 "node": {
						 "exampleCompositions" : [ "CountCharactersInGreetings.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoText) texts,
		VuoInputData(VuoText) separator,
		VuoInputData(VuoBoolean, {"default": true}) includeEmptyParts,
		VuoOutputData(VuoText) compositeText
)
{
	*compositeText = VuoText_appendWithSeparator(texts, separator, includeEmptyParts);
}
