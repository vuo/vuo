/**
 * @file
 * vuo.text.countCharacters node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Count Characters",
					 "keywords" : [ "letter", "length", "size", "strlen", "string" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "CountCharactersInGreetings.vuo", "RevealWord.vuo" ]
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
