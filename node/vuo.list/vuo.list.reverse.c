/**
 * @file
 * vuo.list.reverse node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Reverse List",
					  "keywords" : [ "backward", "invert", "reorder" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ReverseGradient.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoOutputData(VuoList_VuoGenericType1) reversedList
)
{
	*reversedList = VuoListCopy_VuoGenericType1(list);
	VuoListReverse_VuoGenericType1(*reversedList);
}
