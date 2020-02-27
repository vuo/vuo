/**
 * @file
 * vuo.rss.find.author node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoRssItem.h"

#include "VuoList_VuoRssItem.h"

VuoModuleMetadata({
	"title" : "Find RSS Items by Author",
	"keywords" : [ "filter", "search", "name", "creator", "byline" ],
	"version" : "2.0.0",
	"node" : {
		"exampleCompositions" : []
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoRssItem) items,
	VuoInputData(VuoText, { "default" : "*" }) author,
	VuoOutputData(VuoList_VuoRssItem) foundItems)
{
	*foundItems = VuoListCreate_VuoRssItem();
	int count = VuoListGetCount_VuoRssItem(items);

	for (int i = 1; i < count+1; i++)
	{
		VuoRssItem item = VuoListGetValue_VuoRssItem(items, i);

		if (VuoText_compare(item.author, (VuoTextComparison){VuoTextComparison_MatchesWildcard, true, ""}, author))
			VuoListAppendValue_VuoRssItem(*foundItems, item);
	}
}
