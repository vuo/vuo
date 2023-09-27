/**
 * @file
 * vuo.tree.find.xpath node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Find Subtrees using XPath",
					  "keywords" : [
						"search", "filter", "seek", "path",
						"xml", "json", "element", "tag", "object"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ListCountries.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoInputData(VuoText) expression,
		VuoOutputData(VuoList_VuoTree) foundTrees
)
{
	*foundTrees = VuoTree_findItemsUsingXpath(tree, expression);
}
