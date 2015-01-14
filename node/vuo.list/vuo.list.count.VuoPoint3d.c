/**
 * @file
 * vuo.list.count.point3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleDetails({
					 "name" : "List Count",
					 "description" : "Returns the number of items present in the passed list.",
					 "keywords" : ["count", "amount", "length", "how many"],
					 "version" : "1.0.0",
					 "node" : {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint3d) list,
		VuoOutputData(VuoInteger) count
)
{
	*count = VuoListGetCount_VuoPoint3d(list);
}
