﻿/**
 * @file
 * vuo.text.replace node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Replace Text",
	"keywords": [
		"string", "character", "letter",
		"search", "find", "locate", "where", "place",
		"replace", "substitute", "alter", "change",
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
		VuoInputData(VuoText, {"default":""}) replacement,
		VuoOutputData(VuoText) modifiedText
)
{
	*modifiedText = VuoText_replace(text, textToFind, replacement);
}
