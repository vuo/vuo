/**
 * @file
 * vuo.window.fullscreen node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Change Fullscreen Status",
					 "keywords" : [ "windowed", "screen", "display", "properties" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "ShowWindowsOn2Screens.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":true}) fullscreen,
		VuoInputData(VuoScreen) screen,
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_FullScreen;
	(*property).fullScreen = fullscreen;
	(*property).screen = screen;
}
