/**
 * @file
 * vuo.rss.get node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoRssItem.h"

VuoModuleMetadata({
					  "title" : "Get RSS Item Values",
					  "keywords" : [ ],
					  "version" : "1.0.0",
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
		VuoOutputData(VuoText, {"name":"URL"}) url,
		VuoOutputData(VuoImage) image
)
{
	*title = item.title;
	*author = item.author;
	*description = item.description;
	*url = item.url;
	*image = item.image;
}
