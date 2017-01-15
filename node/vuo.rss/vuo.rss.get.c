/**
 * @file
 * vuo.rss.get node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoRssItem.h"
#include "VuoTime.h"
#include "VuoList_VuoText.h"

VuoModuleMetadata({
					  "title" : "Get RSS Item Values",
					  "keywords" : [ ],
					  "version" : "1.1.0",
					  "node" : {
						  "exampleCompositions" : [ "DisplayRssItems.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoRssItem) item,
		VuoOutputData(VuoText) title,
		VuoOutputData(VuoText) author,
		VuoOutputData(VuoText) description,
		VuoOutputData(VuoList_VuoText) categories,
		VuoOutputData(VuoText, {"name":"URL"}) url,
		VuoOutputData(VuoTime) time,
		VuoOutputData(VuoText, {"name":"Image URL"}) imageUrl,
		VuoOutputData(VuoImage) image
)
{
	*title = item.title;
	*author = item.author;
	*description = item.description;
	*categories = item.categories;
	*url = item.url;
	*time = item.dateTime;
	*imageUrl = item.imageUrl;
	*image = item.image;
}
