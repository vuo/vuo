/**
 * @file
 * VuoTextHtml implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTextHtml.h"

#include <libxml/xpath.h>
#include <libxml/HTMLparser.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoTextHtml",
					  "dependencies" : [
						  "xml2",
						  "z",
						  "VuoText"
					  ]
				  });
#endif

/**
 * Redirect libxml2 errors to Console.
 */
static void VuoXmlError(void *unused, xmlError *error)
{
	char *message = strdup(error->message);
	size_t len = strlen(message);
	if (message[len-1] == '\n')
		message[len-1] = 0;

	VUserLog("Error: %s (line %i)", message, error->line);

	free(message);
}

/**
 * Initialize libxml (hopefully just once) for this process.
 */
static void __attribute__((constructor)) init()
{
	xmlInitParser();
	xmlSetStructuredErrorFunc(NULL, VuoXmlError);
}

/**
 * Returns a new string, removing HTML tags,
 * and converting numeric character references and character entity references to their corresponding character.
 */
VuoText VuoText_removeHtml(VuoText text)
{
	if (!text)
		return NULL;

	// First check whether the text contains stuff that looks like HTML,
	// so we don't waste time parsing it if it won't change anything.
	size_t length = strlen(text);
	bool found = false;
	for (unsigned int i = 0; i < length; ++i)
		if (text[i] == '<' || text[i] == '&')
		{
			found = true;
			break;
		}
	if (!found)
		return VuoText_make(text);

	int options = HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING;
	xmlDocPtr doc = htmlReadDoc((const xmlChar *)text, "", "UTF-8", options);
	if (!doc)
	{
		// Sometimes it works on the second try.
		doc = htmlReadDoc((const xmlChar *)text, "", "UTF-8", options);
		if (!doc)
			return NULL;
	}
	VuoDefer(^{ xmlFreeDoc(doc); });


	// Remove the <style> and <script> tags.
	{
		xmlXPathContextPtr xpathContext = xmlXPathNewContext(doc);
		if (!xpathContext)
			return NULL;
		VuoDefer(^{ xmlXPathFreeContext(xpathContext); });

		xmlXPathObjectPtr xpathObject = xmlXPathEvalExpression((const unsigned char *)"//style|//script", xpathContext);
		if (!xpathObject)
			return NULL;
		VuoDefer(^{ xmlXPathFreeObject(xpathObject); });

		if (!xmlXPathNodeSetIsEmpty(xpathObject->nodesetval))
		{
			for (int i = 0; i < xpathObject->nodesetval->nodeNr; ++i)
			{
				xmlUnlinkNode(xpathObject->nodesetval->nodeTab[i]);
				xmlFree(xpathObject->nodesetval->nodeTab[i]);
			}
		}
	}


	xmlNodePtr root = xmlDocGetRootElement(doc);
	xmlChar *content = xmlNodeGetContent(root);
	VuoDefer(^{ xmlFree(content); });

	return VuoText_make((const char *)content);
}
