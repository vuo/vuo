/**
 * @file
 * vuo.tree.get.content node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Get Tree Content",
					  "keywords" : [
						"xml", "json", "element", "tag", "object"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ListCountries.vuo", "LookUpSisterCities.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoInputData(VuoBoolean, {"default":false}) includeDescendants,
		VuoOutputData(VuoText) content
)
{
	*content = VuoTree_getContent(tree, includeDescendants);
}
