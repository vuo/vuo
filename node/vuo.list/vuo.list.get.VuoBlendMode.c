/**
 * @file
 * vuo.list.get node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoBlendMode.h"
#include "VuoList_VuoBlendMode.h"

VuoModuleMetadata({
					 "title" : "Get Item from List",
					 "description" :
						"<p>Selects one item from a list.</p> \
						<p>`which` is the number of the item to pick (1 for the first item, 2 for the second item, etc.). \
						If `which` is less than 1, the first item is picked. \
						If `which` is greater than the list size, the last item is picked.</li> \
						</ul></p>",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "index" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoBlendMode) list,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1}) which,
		VuoOutputData(VuoBlendMode) item
)
{
	*item = VuoListGetValueAtIndex_VuoBlendMode(list, which);
}
