/**
 * @file
 * vuo.type.tree.value node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "json-c/json.h"
#include "VuoTree.h"

VuoModuleMetadata({
					 "title" : "Convert Tree to Value",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "genericTypes" : {
						 "VuoGenericType1" : {
							"defaultType" : "VuoText",
						 }
					 },
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoOutputData(VuoGenericType1) value
)
{
	json_object *valueAsJson = VuoTree_getContainedValue(tree);
	VuoDefer(^{ json_object_put(valueAsJson); });

	*value = VuoGenericType1_makeFromJson(valueAsJson);
}
