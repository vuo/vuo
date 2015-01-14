/**
 * @file
 * vuo.text.cut node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Cut",
					 "description" :
						 "<p>Outputs part of the input text.</p> \
						 <p>This node uses UTF-8 characters, which include but are not limited to ASCII characters.</p> \
						 <p><ul> \
						 <li>`text` — The original, whole text.</li> \
						 <li>`startIndex` — The position of the first character in the part of `text` to output. \
						 1 is the first character in `text`, 2 is the second character in `text`, etc.</li> \
						 <li>`characterCount` — The number of characters in the part of `text` to output.</li> \
						 </ul></p> \
						 <p>If `startIndex` is less than 1 or `characterCount` goes past the end of `text`, \
						 then the portion of `text` that falls within the range is output. \
						 For example, if `characterCount` is too large, then the part of `text` from `startIndex` to the end is output.</p>",
					 "keywords" : [ "character", "letter", "substring", "part", "piece", "string", "truncate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
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
