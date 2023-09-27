﻿/**
 * @file
 * vuo.text.find node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Find Text",
	"keywords": [
		"string", "character", "letter",
		"search", "locate", "where", "place",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoInputData(VuoText, {"default":"", "name":"Text to Find"}) textToFind,
		VuoOutputData(VuoList_VuoInteger) occurrences
)
{
	*occurrences = VuoText_findOccurrences(text, textToFind);
}
