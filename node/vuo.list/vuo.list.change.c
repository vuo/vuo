/**
 * @file
 * vuo.list.change node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Change Item in List",
					  "keywords" : [ "replace", "place", "insert", "substitute", "modify" ],
					  "version" : "1.1.1",
					  "node" : {
						  "exampleCompositions" : [ "ReplaceColorsInGradient.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1}) position,
		VuoInputData(VuoGenericType1) newItem,
		VuoInputData(VuoBoolean, {"default":false, "name":"Expand List if Needed"}) expandListIfNeeded,
		VuoOutputData(VuoList_VuoGenericType1) modifiedList
)
{
	if (!list && !expandListIfNeeded)
	{
		*modifiedList = NULL;
		return;
	}

	if (list)
		*modifiedList = VuoListCopy_VuoGenericType1(list);
	else
		*modifiedList = VuoListCreate_VuoGenericType1();

	VuoListSetValue_VuoGenericType1(*modifiedList, newItem, MAX(position, 0), expandListIfNeeded);
}
