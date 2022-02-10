/**
 * @file
 * vuo.ui.make.theme.button.rounded node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoUiTheme.h"

VuoModuleMetadata({
					  "title" : "Make Action Button Theme (Rounded)",
					  "keywords" : [
						  "interface", "gui", "user interface", "interact",
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "DisplayControlPanel.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoReal, {"default":0.25, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.01}) minimumWidth,
	VuoInputData(VuoReal, {"default":0.08, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.01}) minimumHeight,
	VuoInputData(VuoFont, {"default":{"fontName":"Avenir-Medium","pointSize":24}}) labelFont,
	VuoInputData(VuoAnchor, {"default":{"horizontalAlignment":"center", "verticalAlignment":"center"}}) labelAnchor,
	VuoInputData(VuoPoint2d, {
		"default":{"x":0.03,"y":0},
		"suggestedMin":{"x":0, "y":0},
		"suggestedMax":{"x":1, "y":1},
		"suggestedStep":{"x":0.01, "y":0.01}
	}) labelPadding,
	VuoInputData(VuoColor, {"default":{"r":1.0, "g":1.0,  "b":1.0,  "a":0.7}}) labelColor,
	VuoInputData(VuoColor, {"default":{"r":1.0, "g":1.0,  "b":1.0,  "a":1.0}, "name":"Label Color: Hovered"}) labelColorHovered,
	VuoInputData(VuoColor, {"default":{"r":1.0, "g":1.0,  "b":1.0,  "a":1.0}, "name":"Label Color: Pressed"}) labelColorPressed,
	VuoInputData(VuoColor, {"default":{"r":0.4, "g":0.4,  "b":0.4,  "a":1.0}}) backgroundColor,
	VuoInputData(VuoColor, {"default":{"r":0.4, "g":0.42, "b":0.44, "a":1.0}, "name":"Background Color: Hovered"}) backgroundColorHovered,
	VuoInputData(VuoColor, {"default":{"r":0.4, "g":0.5,  "b":0.6,  "a":1.0}, "name":"Background Color: Pressed"}) backgroundColorPressed,
	VuoInputData(VuoColor, {"default":{"r":0.46,"g":0.46, "b":0.46, "a":1.0}}) borderColor,
	VuoInputData(VuoColor, {"default":{"r":0.46,"g":0.48, "b":0.49, "a":1.0}, "name":"Border Color: Hovered"}) borderColorHovered,
	VuoInputData(VuoColor, {"default":{"r":0.46,"g":0.55, "b":0.64, "a":1.0}, "name":"Border Color: Pressed"}) borderColorPressed,
	VuoInputData(VuoReal, {"default":0.005, "suggestedMin":0.0, "suggestedMax":0.1, "suggestedStep":0.005}) borderThickness,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) cornerRoundness,
	VuoOutputData(VuoUiTheme) theme
)
{
	*theme = VuoUiTheme_makeButtonRounded(	minimumWidth,
											minimumHeight,
											labelFont,
											labelAnchor,
											labelPadding,
											labelColor,
											labelColorHovered,
											labelColorPressed,
											backgroundColor,
											backgroundColorHovered,
											backgroundColorPressed,
											borderColor,
											borderColorHovered,
											borderColorPressed,
											borderThickness,
											cornerRoundness);
}
