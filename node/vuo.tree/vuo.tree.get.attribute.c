/**
 * @file
 * vuo.tree.get.attribute node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Get Tree Attribute",
					  "keywords" : [
						"xml", "element", "tag", "id"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoInputData(VuoText) attribute,
		VuoOutputData(VuoText) value
)
{
	*value = VuoTree_getAttribute(tree, attribute);
}
