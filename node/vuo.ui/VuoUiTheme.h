/**
 * @file
 * VuoUiTheme C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// @{ List type.
typedef void * VuoList_VuoUiTheme;
#define VuoList_VuoUiTheme_TYPE_DEFINED
/// @}

#include "VuoReal.h"
#include "VuoFont.h"
#include "VuoAnchor.h"
#include "VuoColor.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"
#include "VuoImageText.h"
#include "VuoOrientation.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoUiTheme VuoUiTheme
 * A visual style for UI widgets.
 *
 * @{
 */

/**
 * A visual style for UI widgets.
 */
typedef const struct VuoUiTheme_struct { void *l; } * VuoUiTheme;

VuoUiTheme VuoUiTheme_makeFromJson(struct json_object *js);
struct json_object *VuoUiTheme_getJson(const VuoUiTheme value);
char *VuoUiTheme_getSummary(const VuoUiTheme value);

VuoUiTheme VuoUiTheme_makeGroup(VuoList_VuoUiTheme elements);

VuoUiTheme VuoUiTheme_makeButtonRounded(
	VuoReal minimumWidth,
	VuoReal minimumHeight,
	VuoFont labelFont,
	VuoAnchor labelAnchor,
	VuoPoint2d labelPadding,
	VuoColor labelColor,
	VuoColor labelColorHovered,
	VuoColor labelColorPressed,
	VuoColor backgroundColor,
	VuoColor backgroundColorHovered,
	VuoColor backgroundColorPressed,
	VuoColor borderColor,
	VuoColor borderColorHovered,
	VuoColor borderColorPressed,
	VuoReal borderThickness,
	VuoReal cornerRoundness);

VuoUiTheme VuoUiTheme_makeToggleRounded(VuoFont labelFont,

										VuoColor labelColor,
										VuoColor labelColorHovered,
										VuoColor labelColorPressed,
										VuoColor labelColorToggled,
										VuoColor labelColorToggledAndHovered,

										VuoColor checkmarkColor,
										VuoColor checkmarkColorHovered,
										VuoColor checkmarkColorPressed,

										VuoColor checkmarkBorderColor,
										VuoColor checkmarkBorderColorHovered,
										VuoColor checkmarkBorderColorPressed,

										VuoColor checkboxBackgroundColor,
										VuoColor checkboxBackgroundColorHovered,
										VuoColor checkboxBackgroundColorPressed,
										VuoColor checkboxBackgroundColorToggled,
										VuoColor checkboxBackgroundColorToggledAndHovered,

										VuoColor checkboxBorderColor,
										VuoColor checkboxBorderColorHovered,
										VuoColor checkboxBorderColorPressed,
										VuoColor checkboxBorderColorToggled,
										VuoColor checkboxBorderColorToggledAndHovered,

										VuoReal checkboxBorderThickness,
										VuoReal checkboxCornerRoundness,
										VuoReal marginBetweenCheckboxAndLabel);

VuoUiTheme VuoUiTheme_makeTextFieldRounded(VuoFont font,
										   VuoAnchor textAnchor,
										   VuoPoint2d textPadding,
										   VuoColor textColor,
										   VuoColor textColorHovered,
										   VuoColor textColorActive,
										   VuoColor backgroundColor,
										   VuoColor backgroundColorHovered,
										   VuoColor backgroundColorActive,
										   VuoColor borderColor,
										   VuoColor borderColorHovered,
										   VuoColor borderColorActive,
										   VuoReal borderThickness,
										   VuoColor cursorColor,
										   VuoColor selectionColor,
										   VuoReal cornerRoundness);

VuoUiTheme VuoUiTheme_makeSliderRounded(VuoFont labelFont,
										VuoColor labelColor,
										VuoColor labelColorHovered,

										VuoReal handleWidth,
										VuoReal handleHeight,
										VuoReal handleBorderThickness,
										VuoReal handleCornerRoundness,

										VuoColor handleColor,
										VuoColor handleColorHovered,
										VuoColor handleColorPressed,

										VuoColor handleBorderColor,
										VuoColor handleBorderColorHovered,
										VuoColor handleBorderColorPressed,

										VuoReal trackDepth,
										VuoReal trackBorderThickness,
										VuoReal trackCornerRoundness,

										VuoColor activeTrackColor,
										VuoColor activeTrackColorHovered,

										VuoColor activeTrackBorderColor,
										VuoColor activeTrackBorderColorHovered,

										VuoColor inactiveTrackColor,
										VuoColor inactiveTrackColorHovered,

										VuoColor inactiveTrackBorderColor,
										VuoColor inactiveTrackBorderColorHovered,

										VuoReal marginBetweenTrackAndLabel);


#define VuoUiTheme_SUPPORTS_COMPARISON
bool VuoUiTheme_areEqual(const VuoUiTheme valueA, const VuoUiTheme valueB);
bool VuoUiTheme_isLessThan(const VuoUiTheme valueA, const VuoUiTheme valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoUiTheme_getString(const VuoUiTheme value);
void VuoUiTheme_retain(VuoUiTheme value);
void VuoUiTheme_release(VuoUiTheme value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
