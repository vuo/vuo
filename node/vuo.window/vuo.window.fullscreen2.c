/**
 * @file
 * vuo.window.fullscreen node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowDescription.h"

VuoModuleMetadata({
					 "title" : "Change Screen",
					 "keywords" : [ "windowed", "fullscreen", "status", "move", "display", "properties", "settings" ],
					 "version" : "2.0.0",
					 "node": {
						 "exampleCompositions" : [ "ToggleFullscreen.vuo", "ShowWindowsOn2Screens.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoWindowDescription) windowDescription,
		VuoInputData(VuoBoolean, {"default":true}) fullscreen,
		VuoInputData(VuoScreen, {"default":{"type":"active"}}) screen,
		VuoOutputData(VuoWindowDescription) changedWindowDescription
)
{
	VuoWindowProperty property = VuoWindowProperty_makeFromJson(NULL);
	property.type = VuoWindowProperty_FullScreen;
	property.fullScreen = fullscreen;
	property.screen = screen;
	*changedWindowDescription = VuoWindowDescription_copy(windowDescription);
	VuoWindowDescription_setProperty(*changedWindowDescription, property);
}
