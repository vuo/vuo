/**
 * @file
 * vuo.list.populated node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Is List Populated",
					  "keywords" : [ "amount", "length", "how many", "elements", "number", "size", "empty", "non-empty", "nonempty" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoOutputData(VuoBoolean) populated
)
{
	*populated = (VuoListGetCount_VuoGenericType1(list) >= 1);
}
