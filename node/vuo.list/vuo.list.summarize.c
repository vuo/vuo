/**
 * @file
 * vuo.list.summarize node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTextHtml.h"

VuoModuleMetadata({
	"title": "Summarize List Items",
	"keywords": [
		"convert", "brief", "shorten", "debug", "troubleshoot",
		"text", "string",
		"show", "items", "contents",
	],
	"version": "1.0.0",
	"dependencies": [
		"VuoTextHtml",
	],
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoGenericType1) list,
	VuoOutputData(VuoText) summary)
{
	unsigned long count = VuoListGetCount_VuoGenericType1(list);
	VuoGenericType1 *inputData = VuoListGetData_VuoGenericType1(list);
	VuoList_VuoText summaries = VuoListCreateWithCount_VuoText(count, NULL);
	VuoRetain(summaries);
	VuoText *summariesData = VuoListGetData_VuoText(summaries);
	for (unsigned long i = 0; i < count; ++i)
	{
		char *s = VuoGenericType1_getSummary(inputData[i]);
		summariesData[i] = VuoText_removeHtml(s);
		VuoRetain(summariesData[i]);
		free(s);
	}

	*summary = VuoText_appendWithSeparator(summaries, "\n\n", false);
	VuoRelease(summaries);
}
