/**
 * @file
 * vuo.rss.fetch node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageGet.h"
#include "VuoUrlFetch.h"
#include "VuoRssItem.h"
#include "VuoList_VuoRssItem.h"
#include "VuoTime.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/HTMLparser.h>

VuoModuleMetadata({
					  "title" : "Fetch RSS Items",
					  "keywords" : [ ],
					  "version" : "1.1.0",
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

// some rss feeds embed HTML image tags in CDATA in the description, so the following attempts to
// extract those images.
static VuoText parseDescriptionForImageTags(const char* description)
{
	int options = HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING;
	xmlDocPtr doc = htmlReadDoc((const xmlChar *)description, "", "UTF-8", options);
	if (!doc)
	{
		// Sometimes it works on the second try.
		doc = htmlReadDoc((const xmlChar *)description, "", "UTF-8", options);
		if (!doc)
			return NULL;
	}
	VuoDefer(^{ xmlFreeDoc(doc); });

	xmlXPathContextPtr xpathContext = xmlXPathNewContext(doc);
	if (!xpathContext)
		return NULL;
	VuoDefer(^{ xmlXPathFreeContext(xpathContext); });

	xmlXPathObjectPtr xpathObject = xmlXPathEvalExpression((const unsigned char *)"//img", xpathContext);
	if (!xpathObject)
		return NULL;
	VuoDefer(^{ xmlXPathFreeObject(xpathObject); });

	if (xmlXPathNodeSetIsEmpty(xpathObject->nodesetval))
		return NULL;

	// Use the first image in the description (which is typically the headline image).
	xmlNodePtr imgTag = xpathObject->nodesetval->nodeTab[0];
	xmlChar *src = xmlGetProp(imgTag, (xmlChar *)"src");
	VuoDefer(^{ xmlFree(src); });

	return VuoText_make((const char *)src);
}

VuoText VuoText_makeFromXmlContent(xmlNode *node)
{
	xmlChar *content = xmlNodeGetContent(node);
	VuoText text = VuoText_make((const char *)content);
	xmlFree(content);
	return text;
}

// static void printNodeTree(xmlNodePtr node, unsigned int indent)
// {
// 	fprintf(stderr, "%*s" "%s: %s\n", indent, "",
// 		node->name,
// 		node->children != NULL && node->children->content != NULL ? (const char*)node->children->content : "null");

// 	for(xmlNodePtr ptr = node->children; ptr; ptr = ptr->next)
// 	{
// 		if(ptr->type != XML_ELEMENT_NODE)
// 			continue;

// 		printNodeTree(ptr, indent + 4);
// 	}
// }

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputData(VuoBoolean, {"default":false}) fetchImages,
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
		VUserLog("Error: The feed's root element isn't <rss>.");
		xmlFreeDoc(doc);
		goto fail;
	}

	xmlNodePtr channel;
	for (channel = root->children; channel; channel = channel->next)
		if (channel->type == XML_ELEMENT_NODE && strcmp((const char *)channel->name, "channel") == 0)
			break;

	if (!channel)
	{
		VUserLog("Error: The feed doesn't contain a <channel> element inside its <rss> element.");
		xmlFreeDoc(doc);
		goto fail;
	}

	*items = VuoListCreate_VuoRssItem();
	VuoText feedUrl = NULL;
	for (xmlNodePtr cur = channel->children; cur; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE || cur->ns)
			continue;

		if (strcmp((const char *)cur->name, "title") == 0)
			*title = VuoText_makeFromXmlContent(cur);
		else if (strcmp((const char *)cur->name, "description") == 0)
			*description = VuoText_makeFromXmlContent(cur);
		else if (strcmp((const char *)cur->name, "link") == 0)
			feedUrl = VuoText_makeFromXmlContent(cur);
		else if (strcmp((const char *)cur->name, "item") == 0)
		{
			VuoRssItem rssItem = { NULL, NULL, NULL, NULL, NAN, NULL, NULL, VuoListCreate_VuoText() };

			for (xmlNodePtr itemCur = cur->children; itemCur; itemCur = itemCur->next)
			{
				if (itemCur->type != XML_ELEMENT_NODE)
					continue;

				if (!itemCur->ns && strcmp((const char *)itemCur->name, "title") == 0)
					rssItem.title = VuoText_makeFromXmlContent(itemCur);
				else if ((!itemCur->ns && strcmp((const char *)itemCur->name, "author") == 0)
						 || (itemCur->ns && strcmp((const char *)itemCur->ns->prefix, "dc") == 0 && strcmp((const char *)itemCur->name, "creator") == 0))
					rssItem.author = VuoText_makeFromXmlContent(itemCur);
				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "description") == 0)
				{
					rssItem.description = VuoText_makeFromXmlContent(itemCur);

					// Sometimes images are embedded in the <description> tag.
					// If there are already images from explicit tags (e.g., <enclosure>), those should take precedence.
					if (!rssItem.imageUrl)
						rssItem.imageUrl = parseDescriptionForImageTags(rssItem.description);
				}
				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "link") == 0)
				{
					rssItem.url = VuoText_makeFromXmlContent(itemCur);

					// If link is site-relative, prepend the site URL.
					// Example: http://www.nature.org/photos-and-video/photography/daily-nature-photo/rss/index.htm
					if (feedUrl && rssItem.url && rssItem.url[0] == '/')
					{
						VuoText scheme;
						VuoText host;
						if (VuoUrl_getParts(feedUrl, &scheme, NULL, &host, NULL, NULL, NULL, NULL))
						{
							char *fullUrl = VuoText_format("%s://%s%s", scheme, host, rssItem.url);
							VuoRetain(scheme);
							VuoRelease(scheme);
							VuoRetain(host);
							VuoRelease(host);
							VuoRetain(rssItem.url);
							VuoRelease(rssItem.url);
							rssItem.url = VuoText_make(fullUrl);
							free(fullUrl);
						}
					}
				}
				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "pubDate") == 0)
				{
					VuoText t = VuoText_makeFromXmlContent(itemCur);
					VuoRetain(t);
					rssItem.dateTime = VuoTime_makeFromRFC822(t);
					VuoRelease(t);
				}
				else if (!itemCur->ns && strcmp((const char *)itemCur->name, "enclosure") == 0 && itemCur->properties)
				{
					for (xmlAttrPtr attrCur = itemCur->properties; attrCur; attrCur = attrCur->next)
						if (strcmp((const char *)attrCur->name, "url") == 0 && attrCur->children && attrCur->children->type == XML_TEXT_NODE)
						{
							// Overwrite the old image, so we keep the last one we find (which is typically the highest quality).
							if (rssItem.imageUrl)
							{
								VuoRetain(rssItem.imageUrl);
								VuoRelease(rssItem.imageUrl);
							}
							rssItem.imageUrl = VuoText_make((const char *)attrCur->children->content);
						}
				}
				else if (itemCur->ns && strcmp((const char *)itemCur->ns->prefix, "media") == 0 && strcmp((const char *)itemCur->name, "content") == 0 && itemCur->properties)
				{
					for (xmlAttrPtr attrCur = itemCur->properties; attrCur; attrCur = attrCur->next)
						if (strcmp((const char *)attrCur->name, "url") == 0 && attrCur->children && attrCur->children->type == XML_TEXT_NODE)
						{
							// Overwrite the old image, so we keep the last one we find (which is typically the highest quality).
							if (rssItem.imageUrl)
							{
								VuoRetain(rssItem.imageUrl);
								VuoRelease(rssItem.imageUrl);
							}
							rssItem.imageUrl = VuoText_make((const char *)attrCur->children->content);
						}
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
								{
									// Overwrite the old image, so we keep the last one we find (which is typically the highest quality).
									if (rssItem.imageUrl)
									{
										VuoRetain(rssItem.imageUrl);
										VuoRelease(rssItem.imageUrl);
									}
									rssItem.imageUrl = VuoText_make((const char *)attrCur->children->content);
								}
						}
					}
				}
				else if(!itemCur->ns && strcmp((const char*)itemCur->name, "image") == 0 && itemCur->children)
				{
					for(xmlNodePtr imgCur = itemCur->children; imgCur; imgCur = imgCur->next)
						if (strcmp((const char*)imgCur->name, "url") == 0)
						{
							// Overwrite the old image, so we keep the last one we find (which is typically the highest quality).
							if (rssItem.imageUrl)
							{
								VuoRetain(rssItem.imageUrl);
								VuoRelease(rssItem.imageUrl);
							}
							rssItem.imageUrl = VuoText_makeFromXmlContent(imgCur);
						}
				}
				else if(!itemCur->ns && strcmp((const char*)itemCur->name, "category") == 0)
					VuoListAppendValue_VuoText(rssItem.categories, VuoText_makeFromXmlContent(itemCur));
			}

			if (fetchImages && rssItem.imageUrl)
				rssItem.image = VuoImage_get(rssItem.imageUrl);

			VuoListAppendValue_VuoRssItem(*items, rssItem);
		}
	}

	if (feedUrl)
	{
		VuoRetain(feedUrl);
		VuoRelease(feedUrl);
	}

	xmlFreeDoc(doc);
	return;

fail:
	*title = NULL;
	*description = NULL;
	*items = NULL;
}
