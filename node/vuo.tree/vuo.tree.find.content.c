/**
 * @file
 * vuo.tree.find.content node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Find Subtrees with Content",
					  "keywords" : [
						"search", "filter", "seek",
						"xml", "json", "element", "tag", "object",
						"glob", "wildcard",
						"grep", "regex", "regular expression",
					  ],
					  "version" : "1.1.0",
					  "node" : {
						  "exampleCompositions" : [ "LookUpSisterCities.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoInputData(VuoText) content,
		VuoInputData(VuoTextComparison, {"default":{"type":"equals","isCaseSensitive":false}}) comparison,
		VuoInputData(VuoBoolean, {"default":true}) includeDescendants,
		VuoOutputData(VuoList_VuoTree) foundTrees
)
{
	*foundTrees = VuoTree_findItemsWithContent(tree, content, comparison, includeDescendants);
}
