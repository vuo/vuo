/**
 * @file
 * vuo.tree.find.name node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Find Subtrees with Name",
					  "keywords" : [
						"search", "filter", "seek",
						"xml", "json", "element", "tag", "object",
						"glob", "wildcard",
						"grep", "regex", "regular expression",
					  ],
					  "version" : "1.1.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoInputData(VuoText) name,
		VuoInputData(VuoTextComparison, {"default":{"type":"equals","isCaseSensitive":false}}) comparison,
		VuoInputData(VuoBoolean, {"default":true}) includeDescendants,
		VuoOutputData(VuoList_VuoTree) foundTrees
)
{
	*foundTrees = VuoTree_findItemsWithName(tree, name, comparison, includeDescendants);
}
