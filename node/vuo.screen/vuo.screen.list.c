/**
 * @file
 * vuo.screen.list node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoScreenCommon.h"

VuoModuleMetadata({
					  "title" : "List Screens",
					  "keywords" : [ "display", "monitor", "device" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : true
					  },
					  "dependencies" : [
						  "VuoScreenCommon"
					  ]
				 });

void nodeEvent
(
		VuoOutputData(VuoList_VuoScreen) screens
)
{
	*screens = VuoScreen_getList();
}
