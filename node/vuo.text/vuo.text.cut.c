/**
 * @file
 * vuo.text.cut node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Cut Text",
					 "keywords" : [ "character", "letter", "substring", "part", "piece", "string", "truncate", "trim", "subrange", "size" ],
					 "version" : "2.0.0",
					 "node": {
						 "exampleCompositions" : [ "RevealWords.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) startPosition,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) characterCount,
		VuoOutputData(VuoText) partialText
)
{
	*partialText = VuoText_substring(text, startPosition, characterCount);
}
