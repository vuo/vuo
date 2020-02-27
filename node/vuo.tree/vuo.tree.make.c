/**
 * @file
 * vuo.tree.make node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Make Tree",
					  "keywords" : [
						"hierarchy", "hierarchical", "structure", "parent", "descendant", "leaf",
						"xml", "json", "html", "dom", "element", "tag", "object"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "RecordMouseClicksToJsonFile.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoInputData(VuoDictionary_VuoText_VuoText) attributes,
		VuoInputData(VuoText) content,
		VuoInputData(VuoList_VuoTree) children,
		VuoOutputData(VuoTree) tree
)
{
	*tree = VuoTree_make(name, attributes, content, children);
}
