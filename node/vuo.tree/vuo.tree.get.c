/**
 * @file
 * vuo.tree.get node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Get Tree Values",
					  "keywords" : [
						"xml", "json", "element", "tag", "object",
						"descendant"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoDictionary_VuoText_VuoText) attributes,
		VuoOutputData(VuoText) content,
		VuoOutputData(VuoList_VuoTree) children
)
{
	*name = VuoTree_getName(tree);
	*attributes = VuoTree_getAttributes(tree);
	*content = VuoTree_getContent(tree, false);
	*children = VuoTree_getChildren(tree);
}
