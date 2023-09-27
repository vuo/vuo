/**
 * @file
 * vuo.ui.make.theme.toggle.rounded node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoUiTheme.h"

VuoModuleMetadata({
	"title" : "Make Toggle Button Theme (Rounded)",
	"keywords" : [
		"interface", "gui", "user interface", "interact",
	],
	"version" : "1.0.0",
	"node" : {
		"exampleCompositions" : [ "DisplayControlPanel.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoFont, { "default" : { "fontName" : "Avenir-Medium", "pointSize" : 24 } }) labelFont,

	VuoInputData(VuoColor, { "default" : { "r" : 0.7, "g" : 0.7, "b" : 0.7, "a" : 0.7 } }) labelColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.7, "g" : 0.7, "b" : 0.7, "a" : 0.8 }, "name" : "Label Color: Hovered" }) labelColorHovered,
	VuoInputData(VuoColor, { "default" : { "r" : 0.7, "g" : 0.7, "b" : 0.7, "a" : 0.9 }, "name" : "Label Color: Pressed" }) labelColorPressed,
	VuoInputData(VuoColor, { "default" : { "r" : 0.7, "g" : 0.7, "b" : 0.7, "a" : 0.8 }, "name" : "Label Color: Toggled" }) labelColorToggled,
	VuoInputData(VuoColor, { "default" : { "r" : 0.7, "g" : 0.7, "b" : 0.7, "a" : 0.9 }, "name" : "Label Color: Toggled and Hovered" }) labelColorToggledAndHovered,

	VuoInputData(VuoColor, { "default" : { "r" : 1.0, "g" : 1.0, "b" : 1.0, "a" : 0.9 } }) checkmarkColor,
	VuoInputData(VuoColor, { "default" : { "r" : 1.0, "g" : 1.0, "b" : 1.0, "a" : 1.0 }, "name" : "Checkmark Color: Hovered" }) checkmarkColorHovered,
	VuoInputData(VuoColor, { "default" : { "r" : 1.0, "g" : 1.0, "b" : 1.0, "a" : 0.9 }, "name" : "Checkmark Color: Pressed" }) checkmarkColorPressed,

	VuoInputData(VuoColor, { "default" : { "r" : 0.0, "g" : 0.0, "b" : 0.0, "a" : 0.5 } }) checkmarkBorderColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.0, "g" : 0.0, "b" : 0.0, "a" : 0.5 }, "name" : "Checkmark Border Color: Hovered" }) checkmarkBorderColorHovered,
	VuoInputData(VuoColor, { "default" : { "r" : 0.0, "g" : 0.0, "b" : 0.0, "a" : 1.0 }, "name" : "Checkmark Border Color: Pressed" }) checkmarkBorderColorPressed,

	VuoInputData(VuoColor, { "default" : { "r" : 0.4, "g" : 0.4,  "b" : 0.4,  "a" : 1.0 } }) checkboxBackgroundColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.43,"g" : 0.43, "b" : 0.43, "a" : 1.0 }, "name" : "Checkbox Background Color: Hovered" }) checkboxBackgroundColorHovered,
	VuoInputData(VuoColor, { "default" : { "r" : 0.4, "g" : 0.5,  "b" : 0.6,  "a" : 1.0 }, "name" : "Checkbox Background Color: Pressed" }) checkboxBackgroundColorPressed,
	VuoInputData(VuoColor, { "default" : { "r" : 0.4, "g" : 0.6,  "b" : 0.8,  "a" : 1.0 }, "name" : "Checkbox Background Color: Toggled" }) checkboxBackgroundColorToggled,
	VuoInputData(VuoColor, { "default" : { "r" : 0.4, "g" : 0.62, "b" : 0.84, "a" : 1.0 }, "name" : "Checkbox Background Color: Toggled and Hovered" }) checkboxBackgroundColorToggledAndHovered,

	VuoInputData(VuoColor, { "default" : { "r" : 0.46,"g" : 0.46, "b" : 0.46, "a" : 1.0 } }) checkboxBorderColor,
	VuoInputData(VuoColor, { "default" : { "r" : 0.46,"g" : 0.48, "b" : 0.49, "a" : 1.0 }, "name" : "Checkbox Border Color: Hovered" }) checkboxBorderColorHovered,
	VuoInputData(VuoColor, { "default" : { "r" : 0.46,"g" : 0.55, "b" : 0.64, "a" : 1.0 }, "name" : "Checkbox Border Color: Pressed" }) checkboxBorderColorPressed,
	VuoInputData(VuoColor, { "default" : { "r" : 0.46,"g" : 0.64, "b" : 0.82, "a" : 1.0 }, "name" : "Checkbox Border Color: Toggled" }) checkboxBorderColorToggled,
	VuoInputData(VuoColor, { "default" : { "r" : 0.46,"g" : 0.66, "b" : 0.86, "a" : 1.0 }, "name" : "Checkbox Border Color: Toggled and Hovered" }) checkboxBorderColorToggledAndHovered,

	VuoInputData(VuoReal, { "default" : 0.005, "suggestedMin" : 0.0, "suggestedMax" : 0.1, "suggestedStep" : 0.005 }) checkboxBorderThickness,
	VuoInputData(VuoReal, { "default" : 0.5,   "suggestedMin" : 0.0, "suggestedMax" : 1.0, "suggestedStep" : 0.1 }) checkboxCornerRoundness,

	VuoInputData(VuoReal, { "default" : 0.025, "suggestedMin" : 0.0, "suggestedMax" : 0.1, "suggestedStep" : 0.01, "name" : "Margin between Checkbox and Label" }) marginBetweenCheckboxAndLabel,

	VuoOutputData(VuoUiTheme) theme)
{
	*theme = VuoUiTheme_makeToggleRounded(labelFont,

										  labelColor,
										  labelColorHovered,
										  labelColorPressed,
										  labelColorToggled,
										  labelColorToggledAndHovered,

										  checkmarkColor,
										  checkmarkColorHovered,
										  checkmarkColorPressed,

										  checkmarkBorderColor,
										  checkmarkBorderColorHovered,
										  checkmarkBorderColorPressed,

										  checkboxBackgroundColor,
										  checkboxBackgroundColorHovered,
										  checkboxBackgroundColorPressed,
										  checkboxBackgroundColorToggled,
										  checkboxBackgroundColorToggledAndHovered,

										  checkboxBorderColor,
										  checkboxBorderColorHovered,
										  checkboxBorderColorPressed,
										  checkboxBorderColorToggled,
										  checkboxBorderColorToggledAndHovered,

										  checkboxBorderThickness,
										  checkboxCornerRoundness,
										  marginBetweenCheckboxAndLabel);
}
