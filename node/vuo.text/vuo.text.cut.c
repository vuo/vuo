/**
 * @file
 * vuo.text.cut node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Cut",
					 "keywords" : [ "character", "letter", "substring", "part", "piece", "string", "truncate", "trim" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "RevealWord.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) startIndex,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) characterCount,
		VuoOutputData(VuoText) partialText
)
{
	*partialText = VuoText_substring(text, startIndex, characterCount);
}
