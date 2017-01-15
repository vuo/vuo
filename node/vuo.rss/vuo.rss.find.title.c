/**
 * @file
 * vuo.rss.find.title node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoRssItem.h"
#include "VuoList_VuoRssItem.h"

VuoModuleMetadata({
					  "title" : "Find RSS Items by Title",
					  "keywords" : [ "filter", "name" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  },
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoRssItem) items,
		VuoInputData(VuoText) title,
		VuoOutputData(VuoList_VuoRssItem) foundItems
)
{
	*foundItems = VuoListCreate_VuoRssItem();
	int count = VuoListGetCount_VuoRssItem(items);

	for (int i = 1; i < count+1; i++)
	{
		VuoRssItem item = VuoListGetValue_VuoRssItem(items, i);

		if (VuoText_findFirstOccurrence(item.title, title, 1))
			VuoListAppendValue_VuoRssItem(*foundItems, item);
	}
}
