/**
 * @file
 * vuo.window.fullscreen node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Change Fullscreen Status",
					 "keywords" : [ "windowed", "screen", "display", "properties", "set" ],
					 "version" : "1.0.0",
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ "ToggleFullscreen.vuo", "ShowWindowsOn2Screens.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":true}) fullscreen,
		VuoInputData(VuoScreen, {"default":{"type":"active"}}) screen,
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_FullScreen;
	(*property).fullScreen = fullscreen;
	(*property).screen = screen;
}
