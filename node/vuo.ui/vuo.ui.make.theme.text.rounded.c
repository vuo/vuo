/**
 * @file
 * vuo.ui.make.theme.text.rounded node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoUiTheme.h"
#include "VuoList_VuoUiTheme.h"

VuoModuleMetadata({
	"title": "Make Text Field Theme (Rounded)",
	"keywords": [
		"string",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions" : [ ],
	},
});

void nodeEvent
(
	VuoInputData(VuoFont, {"default":{
		"fontName":"Avenir-Medium",
		"alignment":"left",
		"pointSize":24,
		"color":{"r":1, "g":1, "b":1, "a":1}
	}}) font,
	VuoInputData(VuoAnchor, {"default":{"horizontalAlignment":"left", "verticalAlignment":"top"}}) textAnchor,
	VuoInputData(VuoPoint2d, {
		"default":{"x":0.02,"y":0.01},
		"suggestedMin":{"x":0, "y":0},
		"suggestedMax":{"x":1, "y":1},
		"suggestedStep":{"x":0.01, "y":0.01}
	}) textPadding,
	VuoInputData(VuoColor, {"default":{"r":0.0,  "g":0.0,  "b":0.0,  "a":0.7}}) textColor,
	VuoInputData(VuoColor, {"default":{"r":0.0,  "g":0.0,  "b":0.0,  "a":0.8}, "name":"Text Color: Hovered"}}) textColorHovered,
	VuoInputData(VuoColor, {"default":{"r":0.0,  "g":0.0,  "b":0.0,  "a":1.0}, "name":"Text Color: Active"}}) textColorActive,
	VuoInputData(VuoColor, {"default":{"r":1.0,  "g":1.0,  "b":1.0,  "a":0.5}}) backgroundColor,
	VuoInputData(VuoColor, {"default":{"r":1.0,  "g":1.0,  "b":1.0,  "a":0.6}, "name":"Background Color: Hovered"}}) backgroundColorHovered,
	VuoInputData(VuoColor, {"default":{"r":1.0,  "g":1.0,  "b":1.0,  "a":1.0}, "name":"Background Color: Active"}}) backgroundColorActive,
	VuoInputData(VuoColor, {"default":{"r":0.46, "g":0.46, "b":0.46, "a":1.0}}) borderColor,
	VuoInputData(VuoColor, {"default":{"r":0.46, "g":0.48, "b":0.49, "a":1.0}, "name":"Border Color: Hovered"}) borderColorHovered,
	VuoInputData(VuoColor, {"default":{"r":0.46, "g":0.48, "b":1.0,  "a":1.0}, "name":"Border Color: Active"}) borderColorActive,
	VuoInputData(VuoReal, {"default":0.005, "suggestedMin":0.0, "suggestedMax":0.1, "suggestedStep":0.005}) borderThickness,
	VuoInputData(VuoColor, {"default":{"r":0.0,  "g":0.0,  "b":0.0,  "a":1.0}}) cursorColor,
	VuoInputData(VuoColor, {"default":{"r":0.7,  "g":0.84, "b":1.0,  "a":1.0}}) selectionColor,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) cornerRoundness,
	VuoOutputData(VuoUiTheme) theme
)
{
	*theme = VuoUiTheme_makeTextFieldRounded(font,
											 textAnchor,
											 textPadding,
											 textColor,
											 textColorHovered,
											 textColorActive,
											 backgroundColor,
											 backgroundColorHovered,
											 backgroundColorActive,
											 borderColor,
											 borderColorHovered,
											 borderColorActive,
											 borderThickness,
											 cursorColor,
											 selectionColor,
											 cornerRoundness);
}
