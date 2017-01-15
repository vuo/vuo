/**
 * @file
 * vuo.screen.make.name node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Screen from Name",
					  "keywords" : [ "display", "monitor", "device" ],
					  "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoOutputData(VuoScreen) screen
)
{
	*screen = VuoScreen_makeFromName(name);
}
