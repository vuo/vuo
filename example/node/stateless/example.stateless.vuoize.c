/**
 * @file
 * example.stateless.vuoize node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Vuoize Text",
					 "description" : "Takes a text string and Vuoizes it (by prepending the word 'Vuo').",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [ ],
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText) text,
		VuoOutputData(VuoText) vuoizedText
)
{
	VuoText texts[2] = {
		"Vuo",
		text
	};
	*vuoizedText = VuoText_append(texts, 2);
}
