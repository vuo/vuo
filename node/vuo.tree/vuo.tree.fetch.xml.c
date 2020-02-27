/**
 * @file
 * vuo.tree.fetch.xml node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"
#include "VuoUrlFetch.h"

VuoModuleMetadata({
					  "title" : "Fetch XML Tree",
					  "keywords" : [
						"download", "open", "load", "import", "http", "url", "file", "get", "read", "make",
						"hierarchy", "hierarchical", "structure", "parent", "descendant", "leaf",
						"xml", "json", "html", "dom", "element", "tag", "object",
						"parse", "convert", "read"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoUrlFetch",
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ListCountries.vuo", "LookUpCountry.vuo", "LookUpSisterCities.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputData(VuoBoolean, {"default":false}) includeWhitespace,
		VuoOutputData(VuoTree) tree
)
{
	void *data;
	unsigned int dataLength;
	if (VuoUrl_fetch(url, &data, &dataLength))
	{
		*tree = VuoTree_makeFromXmlText(data, includeWhitespace);
		free(data);
	}
	else
		*tree = VuoTree_makeEmpty();
}
