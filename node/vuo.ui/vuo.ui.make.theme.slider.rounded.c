/**
 * @file
 * vuo.ui.make.theme.slider.rounded node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoUiTheme.h"

VuoModuleMetadata({
	"title" : "Make Slider Theme (Rounded)",
	"keywords" : [
		"interface", "gui", "user interface", "interact",
	],
	"version" : "1.0.0",
	"node" : {
		"exampleCompositions" : [ "DisplayControlPanel.vuo", "DisplayProgressBar.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoFont, { "default" : { "fontName" : "Avenir-Medium", "pointSize" : 24 } }) labelFont,
	VuoInputData(VuoColor, { "default" : { "r" : 0.7, "g" : 0.7, "b" : 0.7, "a" : 0.7 } }) labelColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.7, "g" : 0.7, "b" : 0.7, "a" : 0.8 } }) labelColorHovered,

	VuoInputData(VuoReal, { "default" : 0.06, "suggestedMin" : 0.0, "suggestedMax" : 1.0, "suggestedStep" : 0.01 }) handleWidth,
	VuoInputData(VuoReal, { "default" : 0.06, "suggestedMin" : 0.0, "suggestedMax" : 1.0, "suggestedStep" : 0.01 }) handleHeight,
	VuoInputData(VuoReal, { "default" : 0.005, "suggestedMin" : 0.0, "suggestedMax" : 0.1, "suggestedStep" : 0.005 }) handleBorderThickness,
	VuoInputData(VuoReal, { "default" : 1.0, "suggestedMin" : 0.0, "suggestedMax" : 1.0, "suggestedStep" : 0.01 }) handleCornerRoundness,

	VuoInputData(VuoColor, { "default" : { "r" : 0.40, "g" : 0.50, "b" : 0.60, "a" : 1.0 } }) handleColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.40, "g" : 0.52, "b" : 0.64, "a" : 1.0 }, "name":"Handle Color: Hovered" }) handleColorHovered,
	VuoInputData(VuoColor, { "default" : { "r" : 0.40, "g" : 0.60, "b" : 0.80, "a" : 1.0 }, "name":"Handle Color: Pressed" }) handleColorPressed,

	VuoInputData(VuoColor, { "default" : { "r" : 0.20, "g" : 0.30, "b" : 0.40, "a" : 1.0 } }) handleBorderColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.20, "g" : 0.32, "b" : 0.44, "a" : 1.0 }, "name":"Handle Border Color: Hovered" }) handleBorderColorHovered,
	VuoInputData(VuoColor, { "default" : { "r" : 0.20, "g" : 0.40, "b" : 0.60, "a" : 1.0 }, "name":"Handle Border Color: Pressed" }) handleBorderColorPressed,

	VuoInputData(VuoReal, { "default" : 0.015, "suggestedMin" : 0.0, "suggestedMax" : 1.0, "suggestedStep" : 0.01 }) trackDepth,
	VuoInputData(VuoReal, { "default" : 0.005, "suggestedMin" : 0.0, "suggestedMax" : 0.1, "suggestedStep" : 0.005 }) trackBorderThickness,
	VuoInputData(VuoReal, { "default" : 1.0, "suggestedMin" : 0.0, "suggestedMax" : 1.0, "suggestedStep" : 0.01 }) trackCornerRoundness,

	VuoInputData(VuoColor, { "default" : { "r" : 0.40, "g" : 0.50, "b" : 0.60, "a" : 1.0 } }) activeTrackColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.40, "g" : 0.52, "b" : 0.64, "a" : 1.0 }, "name":"Active Track Color: Hovered" }) activeTrackColorHovered,

	VuoInputData(VuoColor, { "default" : { "r" : 0.20, "g" : 0.30, "b" : 0.40, "a" : 1.0 } }) activeTrackBorderColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.20, "g" : 0.32, "b" : 0.44, "a" : 1.0 }, "name":"Active Track Border Color: Hovered" }) activeTrackBorderColorHovered,

	VuoInputData(VuoColor, { "default" : { "r" : 0.40, "g" : 0.40, "b" : 0.40, "a" : 1.0 } }) inactiveTrackColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.40, "g" : 0.42, "b" : 0.44, "a" : 1.0 }, "name":"Inactive Track Color: Hovered" }) inactiveTrackColorHovered,

	VuoInputData(VuoColor, { "default" : { "r" : 0.20, "g" : 0.20, "b" : 0.20, "a" : 0.9 } }) inactiveTrackBorderColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.24, "g" : 0.24, "b" : 0.24, "a" : 1.0 }, "name":"Inactive Track Border Color: Hovered" }) inactiveTrackBorderColorHovered,

	VuoInputData(VuoReal, { "default" : 0.01, "suggestedMin" : 0.0, "suggestedMax" : 0.1, "suggestedStep" : 0.01, "name":"Margin between Track and Label" }) marginBetweenTrackAndLabel,

	VuoOutputData(VuoUiTheme) theme)
{
	*theme = VuoUiTheme_makeSliderRounded(labelFont,
										  labelColor,
										  labelColorHovered,

										  handleWidth,
										  handleHeight,
										  handleBorderThickness,
										  handleCornerRoundness,

										  handleColor,
										  handleColorHovered,
										  handleColorPressed,

										  handleBorderColor,
										  handleBorderColorHovered,
										  handleBorderColorPressed,

										  trackDepth,
										  trackBorderThickness,
										  trackCornerRoundness,

										  activeTrackColor,
										  activeTrackColorHovered,

										  activeTrackBorderColor,
										  activeTrackBorderColorHovered,

										  inactiveTrackColor,
										  inactiveTrackColorHovered,

										  inactiveTrackBorderColor,
										  inactiveTrackBorderColorHovered,

										  marginBetweenTrackAndLabel);
}
