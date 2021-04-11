/**
 * @file
 * vuo.tree.find.attribute node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Find Subtrees with Attribute",
					  "keywords" : [
						"search", "filter", "seek",
						"xml", "element", "tag", "id",
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
		VuoInputData(VuoText) attribute,
		VuoInputData(VuoText) value,
		VuoInputData(VuoTextComparison, {"default":{"type":"equals","isCaseSensitive":false}}) valueComparison,
		VuoInputData(VuoBoolean, {"default":true}) includeDescendants,
		VuoOutputData(VuoList_VuoTree) foundTrees
)
{
	*foundTrees = VuoTree_findItemsWithAttribute(tree, attribute, value, valueComparison, includeDescendants);
}
