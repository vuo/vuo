/**
 * @file
 * vuo.text.countCharacters node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Count Characters",
					 "description" :
						 "<p>Gives the number of characters in the text.</p> \
						 <p>This node uses UTF-8 characters, which include but are not limited to ASCII characters.</p>",
					 "keywords" : [ "letter", "length", "size", "strlen", "string" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoOutputData(VuoInteger) characterCount
)
{
	*characterCount = VuoText_length(text);
}
