/**
 * @file
 * vuo.window.style node implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoWindowDescription.h"

VuoModuleMetadata({
	"title": "Change Window Style",
	"keywords": [
		"light mode", "dark mode", "force", "override",
		"no titlebar", "hide", "hidden", "remove",
		"unified", "full size content view",
		"borderless", "outline", "drop shadow",
		"properties", "settings",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoWindowDescription) windowDescription,
	VuoInputData(VuoInteger, {"menuItems":[
		{"value":0, "name":"Automatic"},
		{"value":1, "name":"Light"},
		{"value":2, "name":"Dark"},
	], "default":0}) appearance,
	VuoInputData(VuoBoolean, {"default":true}) showTitlebar,
	VuoInputData(VuoBoolean, {"default":false}) transparentTitlebar,
	VuoInputData(VuoBoolean, {"default":true}) showShadow,
	VuoOutputData(VuoWindowDescription) changedWindowDescription
)
{
	*changedWindowDescription = VuoWindowDescription_copy(windowDescription);
	VuoWindowProperty wp;
	bzero(&wp, sizeof(VuoWindowProperty));
	wp.type = VuoWindowProperty_Style;
	wp.style = (VuoWindowStyle){
		.appearance          = appearance,
		.showTitlebar        = showTitlebar,
		.transparentTitlebar = transparentTitlebar,
		.showShadow          = showShadow,
	};
	VuoWindowDescription_setProperty(*changedWindowDescription, wp);
}
