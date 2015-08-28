/**
 * @file
 * vuo.screen.get node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Get Screen Values",
					  "keywords" : [ "display", "monitor", "device" ],
					  "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoScreen) screen,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoPoint2d) topLeft,
		VuoOutputData(VuoInteger) width,
		VuoOutputData(VuoInteger) height,
		VuoOutputData(VuoInteger) dpiHorizontal,
		VuoOutputData(VuoInteger) dpiVertical
)
{
	*name = screen.name;
	*topLeft = screen.topLeft;
	*width = screen.width;
	*height = screen.height;
	*dpiHorizontal = screen.dpiHorizontal;
	*dpiVertical = screen.dpiVertical;
}
