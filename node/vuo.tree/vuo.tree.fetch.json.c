/**
 * @file
 * vuo.tree.fetch.json node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTree.h"
#include "VuoUrlFetch.h"

VuoModuleMetadata({
					  "title" : "Fetch JSON Tree",
					  "keywords" : [
						"download", "open", "load", "import", "http", "url", "file", "get", "read", "make",
						"hierarchy", "hierarchical", "structure", "parent", "descendant", "leaf",
						"xml", "json", "html", "dom", "element", "tag", "object",
						"parse", "convert", "read"
					  ],
					  "version" : "1.0.1",
					  "dependencies" : [
						  "VuoUrlFetch",
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ListCountries.vuo", "LookUpCountry.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoOutputData(VuoTree) tree
)
{
	void *data;
	unsigned int dataLength;
	if (VuoUrl_fetch(url, &data, &dataLength))
	{
		*tree = VuoTree_makeFromJsonText(data);
		free(data);
	}
	else
		*tree = VuoTree_makeEmpty();
}
