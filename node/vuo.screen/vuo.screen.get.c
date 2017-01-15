/**
 * @file
 * vuo.screen.get node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Get Screen Values",
					  "keywords" : [ "display", "monitor", "device", "information" ],
					  "version" : "1.1.0"
				 });

void nodeEvent
(
		VuoInputData(VuoScreen, {"default":{"type":"active"}}) screen,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoPoint2d) topLeft,
		VuoOutputData(VuoInteger) width,
		VuoOutputData(VuoInteger) height,
		VuoOutputData(VuoInteger, {"name":"DPI Horizontal"}) dpiHorizontal,
		VuoOutputData(VuoInteger, {"name":"DPI Vertical"}) dpiVertical
)
{
	VuoScreen realizedScreen;
	if (VuoScreen_realize(screen, &realizedScreen))
	{
		*name          = realizedScreen.name;
		*topLeft       = realizedScreen.topLeft;
		*width         = realizedScreen.width;
		*height        = realizedScreen.height;
		*dpiHorizontal = realizedScreen.dpiHorizontal;
		*dpiVertical   = realizedScreen.dpiVertical;
	}
}
