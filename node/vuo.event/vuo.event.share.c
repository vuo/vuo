/**
 * @file
 * vuo.event.share node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Share Event",
					  "keywords" : [ "splitter", "input splitter", "output splitter", "hold", "pass" ],
					  "version" : "1.0.0"
				  });

void nodeEvent
(
		VuoInputEvent({"hasPortAction":false}) event,
		VuoOutputEvent() sameEvent
)
{
}
