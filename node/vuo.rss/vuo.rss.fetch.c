/**
 * @file
 * vuo.rss.fetch node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageGet.h"
#include "VuoUrlFetch.h"
#include "VuoRssItem.h"
#include "VuoList_VuoRssItem.h"

#include <libxml/tree.h>
#include <libxml/parser.h>

VuoModuleMetadata({
					  "title" : "Fetch RSS Items",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "DisplayRssItems.vuo" ]
					  },
					  "dependencies" : [
						  "xml2",
						  "VuoImageGet",
						  "VuoUrlFetch"
					  ]
				 });

static void __attribute__((constructor)) init()
{
	xmlInitParser();
}

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputData(VuoBoolean, {"default":true}) fetchImages,
		VuoOutputData(VuoText) title,
		VuoOutputData(VuoText) description,
		VuoOutputData(VuoList_VuoRssItem) items
)
{
	void *data;
	unsigned int dataLength;
	if (!VuoUrl_fetch(url, &data, &dataLength))
		goto fail;


	xmlDocPtr doc = xmlParseMemory(data, dataLength);
	if (!doc)
		goto fail;

	xmlNodePtr root = xmlDocGetRootElement(doc);
	if (!root || strcmp((const char *)root->name, "rss") != 0)
	{
		VLog("Error: The feed's root element isn't <rss>.");
		xmlFreeDoc(doc);
		goto fail;
	}

	xmlNodePtr channel;
	for (channel = root->children; channel; channel = channel->next)
		if (channel->type == XML_ELEMENT_NODE && strcmp((const char *)channel->name, "channel") == 0)
			break;

	if (!channel)
	{
		VLog("Error: The feed doesn't contain a <channel> element inside its <rss> element.");
		xmlFreeDoc(doc);
		goto fail;
	}


	*items = VuoListCreate_VuoRssItem();
	for (xmlNodePtr cur = channel->children; cur; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE || cur->ns)
			continue;

		if (strcmp((const char *)cur->name, "title") == 0 && cur->children && cur->children->type == XML_TEXT_NODE)
			*title = VuoText_make((const char *)cur->children->content);
		else if (strcmp((const char *)cur->name, "description") == 0 && cur->children && cur->children->type == XML_TEXT_NODE)
			*description = VuoText_make((const char *)cur->children->content);
		else if (strcmp((const char *)cur->name, "item") == 0)
		{
			VuoRssItem rssItem = {NULL, NULL, NULL, NULL, NULL};
			const char *image = NULL;

			for (xmlNodePtr itemCur = cur->children; itemCur; itemCur = itemCur->next)
			{
				if (itemCur->type != XML_ELEMENT_NODE)
					continue;

				if (!itemCur->ns && strcmp((const char *)itemCur->name, "title") == 0 && itemCur->children && itemCur->children->type == XML_TEXT_NODE)
					rssItem.title = VuoText_make((const char *)itemCur->children->content);
				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "author") == 0 && itemCur->children && itemCur->children->type == XML_TEXT_NODE)
					rssItem.author = VuoText_make((const char *)itemCur->children->content);
				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "description") == 0 && itemCur->children && itemCur->children->type == XML_TEXT_NODE)
					rssItem.description = VuoText_make((const char *)itemCur->children->content);
				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "link") == 0 && itemCur->children && itemCur->children->type == XML_TEXT_NODE)
					rssItem.url = VuoText_make((const char *)itemCur->children->content);
//				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "pubDate") == 0 && itemCur->children && itemCur->children->type == XML_TEXT_NODE)
//					fprintf(stderr, "\tpubDate: %s\n", itemCur->children->content);
				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "enclosure") == 0 && itemCur->properties)
				{
					for (xmlAttrPtr attrCur = itemCur->properties; attrCur; attrCur = attrCur->next)
						if (strcmp((const char *)attrCur->name, "url") == 0 && attrCur->children && attrCur->children->type == XML_TEXT_NODE)
							image = (const char *)attrCur->children->content;
				}
				else if (itemCur->ns && strcmp((const char *)itemCur->ns->prefix, "media") == 0 && strcmp((const char *)itemCur->name, "content") == 0)
				{
					for (xmlAttrPtr attrCur = itemCur->properties; attrCur; attrCur = attrCur->next)
						if (strcmp((const char *)attrCur->name, "url") == 0 && attrCur->children && attrCur->children->type == XML_TEXT_NODE)
							image = (const char *)attrCur->children->content;
				}
				else if (itemCur->ns && strcmp((const char *)itemCur->ns->prefix, "media") == 0 && strcmp((const char *)itemCur->name, "group") == 0 && itemCur->children)
				{
					for (xmlNodePtr groupItemCur = itemCur->children; groupItemCur; groupItemCur = groupItemCur->next)
					{
						if (groupItemCur->type != XML_ELEMENT_NODE)
							continue;

						if (strcmp((const char *)groupItemCur->name, "content") == 0)
						{
							for (xmlAttrPtr attrCur = groupItemCur->properties; attrCur; attrCur = attrCur->next)
								if (strcmp((const char *)attrCur->name, "url") == 0 && attrCur->children && attrCur->children->type == XML_TEXT_NODE)
									// Overwrite the old image, so we keep the last one we find (which is typically the highest quality).
									image = (const char *)attrCur->children->content;
						}
					}
				}
			}

			if (fetchImages && image)
				rssItem.image = VuoImage_get(image);

			VuoListAppendValue_VuoRssItem(*items, rssItem);
		}
	}

	xmlFreeDoc(doc);
	return;

fail:
	*title = NULL;
	*description = NULL;
	*items = NULL;
}
