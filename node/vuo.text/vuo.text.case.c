﻿/**
 * @file
 * vuo.text.case node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Change Case",
					 "keywords" : [
						 "character", "letter", "substring", "string",
						 "uppercase", "uc", "toupper", "caps", "capitalize",
						 "lowercase", "lc", "tolower",
					 ],
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
