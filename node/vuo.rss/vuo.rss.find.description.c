/**
 * @file
 * vuo.rss.find.description node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRssItem.h"
#include "VuoList_VuoRssItem.h"

VuoModuleMetadata({
					  "title" : "Find RSS Items by Description",
					  "keywords" : [ "filter", "search", "summary", "teaser", "body", "text" ],
					  "version" : "1.0.0",
					  "node" : {
						  "isDeprecated": true,
						  "exampleCompositions" : [ ]
					  },
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoRssItem) items,
		VuoInputData(VuoText) description,
		VuoOutputData(VuoList_VuoRssItem) foundItems
)
{
	*foundItems = VuoListCreate_VuoRssItem();
	int count = VuoListGetCount_VuoRssItem(items);

	for (int i = 1; i < count+1; i++)
	{
		VuoRssItem item = VuoListGetValue_VuoRssItem(items, i);

		if (VuoText_findFirstOccurrence(item.description, description, 1))
			VuoListAppendValue_VuoRssItem(*foundItems, item);
	}
}
