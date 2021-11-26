/**
 * @file
 * vuo.data.summarize node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTextHtml.h"

VuoModuleMetadata({
	"title": "Summarize Value",
	"keywords": [
		"convert", "brief", "shorten", "debug", "troubleshoot",
		"text", "string",
		"real", "number", // since we deprecated vuo.type.real.text
	],
	"version": "1.0.1",
	"dependencies": [
		"VuoTextHtml",
	],
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent
(
		VuoInputData(VuoGenericType1) value,
		VuoOutputData(VuoText) summary
)
{
	char *s = VuoGenericType1_getSummary(value);
	*summary = VuoText_removeHtml(s);
	free(s);
}
