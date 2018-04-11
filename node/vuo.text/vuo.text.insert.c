/**
 * @file
 * vuo.text.cut node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Insert Text",
					 "keywords" : [ "character", "letter", "splice", "prepend", "append",
						 "concatenate", "combine", "join", "merge", "string", "place", "middle" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) position,
		VuoInputData(VuoText, {"default":""}) newText,
		VuoOutputData(VuoText) compositeText
)
{
	*compositeText = VuoText_insert(text, position, newText);
}
