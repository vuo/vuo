/**
 * @file
 * vuo.text.case node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Change Case",
					 "keywords" : [ "character", "letter", "substring", "string", "upper", "lower", "toupper", "tolower", "uc", "lc" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoInputData(VuoTextCase, {"name":"Case", "default":"sentence"}) textCase,
		VuoOutputData(VuoText) casedText
)
{
	*casedText = VuoText_changeCase(text, textCase);
}
