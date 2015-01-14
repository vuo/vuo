/**
 * @file
 * vuo.list.count.VuoLeapPointable node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"


VuoModuleMetadata({
					  "title" : "Count Items in List",
					  "description" :
						  "<p>Gives the number of items in the list.</p>",
					  "keywords" : [ "amount", "length", "how many", "elements" ],
					  "version" : "1.0.0",
					  "node" : {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoLeapPointable) list,
		VuoOutputData(VuoInteger) itemCount
)
{
	*itemCount = VuoListGetCount_VuoLeapPointable(list);
}
