/**
 * @file
 * vuo.list.count node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Count Items in List",
					  "keywords" : [ "amount", "length", "how many", "elements" ],
					  "version" : "1.0.0",
					  "node" : {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoOutputData(VuoInteger) itemCount
)
{
	*itemCount = VuoListGetCount_VuoGenericType1(list);
}
