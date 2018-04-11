/**
 * @file
 * vuo.app.stopComposition node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Stop Composition",
					 "keywords" : [ "quit", "close", "shut down", "end", "halt", "terminate" ],
					 "version" : "1.0.0",
					 "dependencies" : [ ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputEvent() stop
)
{
	if (stop)
		VuoStopComposition();
}
