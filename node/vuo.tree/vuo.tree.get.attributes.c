/**
 * @file
 * vuo.tree.get.attributes node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Get Tree Attributes",
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
		VuoOutputData(VuoDictionary_VuoText_VuoText) attributes
)
{
	*attributes = VuoTree_getAttributes(tree);
}
