/**
 * @file
 * vuo.text.removeHtml node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTextHtml.h"

VuoModuleMetadata({
	"title": "Remove HTML",
	"keywords": [
		"string",
		"strip", "filter", "interpret",
		"xml", "markup", "tags", "character", "entity", "reference",
	],
	"version" : "1.0.0",
	"node": {
		"exampleCompositions": [ "vuo-example://vuo.rss/DisplayRssItems.vuo" ],
	},
	"dependencies": [
		"VuoTextHtml",
	],
});

void nodeEvent
(
		VuoInputData(VuoText) text,
		VuoOutputData(VuoText) modifiedText
)
{
	*modifiedText = VuoText_removeHtml(text);
}
