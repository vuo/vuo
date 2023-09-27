/**
 * @file
 * vuo.tree.find node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTree.h"
#include "VuoList_VuoTree.h"

VuoModuleMetadata({
					  "title" : "Find Subtree Value",
					  "keywords" : [
						"search", "filter", "seek",
						"convert",
						"xml", "json", "element", "tag", "object"
					  ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							 "defaultType" : "VuoText",
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ "LookUpCountry.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoInputData(VuoText) name,
		VuoOutputData(VuoGenericType1) value
)
{
	VuoTextComparison comparison = {VuoTextComparison_Equals, false};
	VuoList_VuoTree foundTrees = VuoTree_findItemsWithName(tree, name, comparison, true);
	VuoLocal(foundTrees);

	VuoTree foundTree = VuoListGetValue_VuoTree(foundTrees, 1);

	json_object *valueAsJson = VuoTree_getContainedValue(foundTree);
	VuoDefer(^{ json_object_put(valueAsJson); });

	*value = VuoGenericType1_makeFromJson(valueAsJson);
}
