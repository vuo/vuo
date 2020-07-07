/**
 * @file
 * vuo.window.level node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowDescription.h"

VuoModuleMetadata({
	"title": "Change Window Level",
	"keywords": [
		"stacking", "order", "layer", "group",
		"background", "lower", "desktop", "backstop", "below", "back",
		"foreground", "raise", "floating", "topmost", "overlay", "above", "front",
		"properties", "settings",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ "ShowWindowLevels.vuo" ],
	},
});

void nodeEvent(
	VuoInputData(VuoWindowDescription) windowDescription,
	VuoInputData(VuoInteger, {"menuItems":[
		{"value": 0, "name":"Background"},
		{"value":10, "name":"Normal"},
		{"value":20, "name":"Floating"},
	], "default":20}) level,
	VuoOutputData(VuoWindowDescription) changedWindowDescription
)
{
	*changedWindowDescription = VuoWindowDescription_copy(windowDescription);
	VuoWindowDescription_setProperty(*changedWindowDescription, (VuoWindowProperty){
		.type = VuoWindowProperty_Level,
		.level = level ==  0 ? VuoWindowLevel_Background :
				(level == 10 ? VuoWindowLevel_Normal :
							   VuoWindowLevel_Floating),
	});
}
