/**
 * @file
 * Private VuoUiThemeButtonRounded implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoUiThemeBase.hh"

#include "VuoSceneText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title": "UI Theme: Button, Rounded",
					  "dependencies": [
						  "VuoBoolean",
						  "VuoColor",
						  "VuoFont",
						  "VuoImage",
						  "VuoImageText",
						  "VuoLayer",
						  "VuoPoint2d",
						  "VuoReal",
						  "VuoAnchor",
						  "VuoHorizontalAlignment",
						  "VuoVerticalAlignment",
						  "VuoSceneText"
					  ],
				  });
#endif
/// @}

/**
 * A theme for button widgets, with the rounded style.
 */
class VuoUiThemeButtonRounded : public VuoUiThemeButton
{
private:
	VuoReal minimumWidth;
	VuoReal minimumHeight;
	VuoFont labelFont;
	VuoAnchor labelPosition;
	VuoPoint2d labelPadding;
	VuoColor labelColor;
	VuoColor labelColorHovered;
	VuoColor labelColorPressed;
	VuoColor backgroundColor;
	VuoColor backgroundColorHovered;
	VuoColor backgroundColorPressed;
	VuoColor borderColor;
	VuoColor borderColorHovered;
	VuoColor borderColorPressed;
	VuoReal borderThickness;
	VuoReal cornerRoundness;

public:
	static std::string type; ///< The subtype's class name.

	/**
	 * Creates a rounded button theme from JSON.
	 */
	static VuoSerializable *makeFromJson(json_object *js)
	{
		return new VuoUiThemeButtonRounded(VuoJson_getObjectValue(VuoReal,    js, "minimumWidth",           0.25),
										   VuoJson_getObjectValue(VuoReal,    js, "minimumHeight",          0.08),
										   VuoJson_getObjectValue(VuoFont,    js, "labelFont",              (VuoFont){VuoText_make("Avenir-Medium"), 24, false, (VuoColor){1,1,1,1}, VuoHorizontalAlignment_Center, 1, 1}),
										   VuoJson_getObjectValue(VuoAnchor,  js, "labelPosition",          VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center)),
										   VuoJson_getObjectValue(VuoPoint2d, js, "labelPadding",           (VuoPoint2d){.03, 0}),
										   VuoJson_getObjectValue(VuoColor,   js, "labelColor",             (VuoColor){1,1,1,.7}),
										   VuoJson_getObjectValue(VuoColor,   js, "labelColorHovered",      (VuoColor){1,1,1,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "labelColorPressed",      (VuoColor){1,1,1,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "backgroundColor",        (VuoColor){.4,.4,.4,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "backgroundColorHovered", (VuoColor){.4,.42,.44,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "backgroundColorPressed", (VuoColor){.4,.5,.6,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "borderColor",            (VuoColor){.46,.46,.46,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "borderColorHovered",     (VuoColor){.46,.48,.49,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "borderColorPressed",     (VuoColor){.46,.55,.64,1}),
										   VuoJson_getObjectValue(VuoReal,    js, "borderThickness",        0.005),
										   VuoJson_getObjectValue(VuoReal,    js, "cornerRoundness",        0.5));
	}

	/**
	 * Creates a theme for button widgets, with the rounded style.
	 */
	VuoUiThemeButtonRounded(VuoReal _minimumWidth,
							VuoReal _minimumHeight,
							VuoFont _labelFont,
							VuoAnchor _labelPosition,
							VuoPoint2d _labelPadding,
							VuoColor _labelColor,
							VuoColor _labelColorHovered,
							VuoColor _labelColorPressed,
							VuoColor _backgroundColor,
							VuoColor _backgroundColorHovered,
							VuoColor _backgroundColorPressed,
							VuoColor _borderColor,
							VuoColor _borderColorHovered,
							VuoColor _borderColorPressed,
							VuoReal _borderThickness,
							VuoReal _cornerRoundness)
	{
		minimumWidth = _minimumWidth;
		minimumHeight = _minimumHeight;
		labelFont = _labelFont;
		VuoFont_retain(labelFont);
		labelPosition = _labelPosition;
		labelPadding = _labelPadding;
		labelColor             = _labelColor;
		labelColorHovered      = _labelColorHovered;
		labelColorPressed      = _labelColorPressed;
		backgroundColor        = _backgroundColor;
		backgroundColorHovered = _backgroundColorHovered;
		backgroundColorPressed = _backgroundColorPressed;
		borderColor            = _borderColor;
		borderColorHovered     = _borderColorHovered;
		borderColorPressed     = _borderColorPressed;
		borderThickness = _borderThickness;
		cornerRoundness = _cornerRoundness;
	}
	~VuoUiThemeButtonRounded()
	{
		VuoFont_release(labelFont);
	}

	/**
	 * Encodes the theme as a JSON object.
	 */
	json_object *getJson()
	{
		json_object *json = VuoSerializable::getJson();
		json_object_object_add(json, "minimumWidth", VuoReal_getJson(minimumWidth));
		json_object_object_add(json, "minimumHeight", VuoReal_getJson(minimumHeight));
		json_object_object_add(json, "labelFont", VuoFont_getJson(labelFont));
		json_object_object_add(json, "labelPosition", VuoAnchor_getJson(labelPosition));
		json_object_object_add(json, "labelPadding", VuoPoint2d_getJson(labelPadding));
		json_object_object_add(json, "labelColor",             VuoColor_getJson(labelColor));
		json_object_object_add(json, "labelColorHovered",      VuoColor_getJson(labelColorHovered));
		json_object_object_add(json, "labelColorPressed",      VuoColor_getJson(labelColorPressed));
		json_object_object_add(json, "backgroundColor",        VuoColor_getJson(backgroundColor));
		json_object_object_add(json, "backgroundColorHovered", VuoColor_getJson(backgroundColorHovered));
		json_object_object_add(json, "backgroundColorPressed", VuoColor_getJson(backgroundColorPressed));
		json_object_object_add(json, "borderColor",            VuoColor_getJson(borderColor));
		json_object_object_add(json, "borderColorHovered",     VuoColor_getJson(borderColorHovered));
		json_object_object_add(json, "borderColorPressed",     VuoColor_getJson(borderColorPressed));
		json_object_object_add(json, "borderThickness", VuoReal_getJson(borderThickness));
		json_object_object_add(json, "cornerRoundness", VuoReal_getJson(cornerRoundness));
		return json;
	}

	/**
	 * Returns a compact string representation of the theme.
	 */
	char *getSummary()
	{
		return strdup("Action Button Theme (Rounded)");
	}

	/**
	 * Returns true if both themes are of the same subtype, and their values are equal.
	 */
	bool operator==(const VuoSerializable &that)
	{
		VuoSerializableEquals(VuoUiThemeButtonRounded);
		return VuoReal_areEqual(minimumWidth, thatSpecialized->minimumWidth)
			&& VuoReal_areEqual(minimumHeight, thatSpecialized->minimumHeight)
			&& VuoFont_areEqual(labelFont, thatSpecialized->labelFont)
			&& VuoAnchor_areEqual(labelPosition, thatSpecialized->labelPosition)
			&& VuoPoint2d_areEqual(labelPadding, thatSpecialized->labelPadding)
			&& VuoColor_areEqual(labelColor,             thatSpecialized->labelColor)
			&& VuoColor_areEqual(labelColorHovered,      thatSpecialized->labelColorHovered)
			&& VuoColor_areEqual(labelColorPressed,      thatSpecialized->labelColorPressed)
			&& VuoColor_areEqual(backgroundColor,        thatSpecialized->backgroundColor)
			&& VuoColor_areEqual(backgroundColorHovered, thatSpecialized->backgroundColorHovered)
			&& VuoColor_areEqual(backgroundColorPressed, thatSpecialized->backgroundColorPressed)
			&& VuoColor_areEqual(borderColor,            thatSpecialized->borderColor)
			&& VuoColor_areEqual(borderColorHovered,     thatSpecialized->borderColorHovered)
			&& VuoColor_areEqual(borderColorPressed,     thatSpecialized->borderColorPressed)
			&& VuoReal_areEqual(borderThickness, thatSpecialized->borderThickness)
			&& VuoReal_areEqual(cornerRoundness, thatSpecialized->cornerRoundness);
	}

	/**
	 * Returns true if this theme sorts before `that` theme.
	 */
	bool operator<(const VuoSerializable &that)
	{
		VuoSerializableLessThan(VuoUiThemeButtonRounded);
		VuoType_returnInequality(VuoReal,    minimumWidth,           thatSpecialized->minimumWidth);
		VuoType_returnInequality(VuoReal,    minimumHeight,          thatSpecialized->minimumHeight);
		VuoType_returnInequality(VuoFont,    labelFont,              thatSpecialized->labelFont);
		VuoType_returnInequality(VuoAnchor,  labelPosition,          thatSpecialized->labelPosition);
		VuoType_returnInequality(VuoPoint2d, labelPadding,           thatSpecialized->labelPadding);
		VuoType_returnInequality(VuoColor,   labelColor,             thatSpecialized->labelColor);
		VuoType_returnInequality(VuoColor,   labelColorHovered,      thatSpecialized->labelColorHovered);
		VuoType_returnInequality(VuoColor,   labelColorPressed,      thatSpecialized->labelColorPressed);
		VuoType_returnInequality(VuoColor,   backgroundColor,        thatSpecialized->backgroundColor);
		VuoType_returnInequality(VuoColor,   backgroundColorHovered, thatSpecialized->backgroundColorHovered);
		VuoType_returnInequality(VuoColor,   backgroundColorPressed, thatSpecialized->backgroundColorPressed);
		VuoType_returnInequality(VuoColor,   borderColor,            thatSpecialized->borderColor);
		VuoType_returnInequality(VuoColor,   borderColorHovered,     thatSpecialized->borderColorHovered);
		VuoType_returnInequality(VuoColor,   borderColorPressed,     thatSpecialized->borderColorPressed);
		VuoType_returnInequality(VuoReal,    borderThickness,        thatSpecialized->borderThickness);
		VuoType_returnInequality(VuoReal,    cornerRoundness,        thatSpecialized->cornerRoundness);
		return false;
	}

	/**
	 * Creates a layer group representing a button with the specified theme and parameters.
	 *
	 *    - borderLayer — the outermost edge
	 *    - backgroundLayer — inset from borderLayer; color changes when hovered or pressed
	 *    - textLayer (optional) — label
	 */
	VuoLayer render(VuoRenderedLayers renderedLayers, VuoText label, VuoPoint2d position, VuoAnchor anchor, bool isHovered, bool isPressed)
	{
		VuoLayer textLayer = nullptr;
		bool hasText = VuoText_length(label) > 0;

		float actualWidth  = minimumWidth;
		float actualHeight = minimumHeight;

		VuoFont f = labelFont;
		if (isPressed)
			f.color = (VuoColor){f.color.r * labelColorPressed.r,
								 f.color.g * labelColorPressed.g,
								 f.color.b * labelColorPressed.b,
								 f.color.a * labelColorPressed.a};
		else if (isHovered)
			f.color = (VuoColor){f.color.r * labelColorHovered.r,
								 f.color.g * labelColorHovered.g,
								 f.color.b * labelColorHovered.b,
								 f.color.a * labelColorHovered.a};
		else
			f.color = (VuoColor){f.color.r * labelColor.r,
								 f.color.g * labelColor.g,
								 f.color.b * labelColor.b,
								 f.color.a * labelColor.a};

		VuoPoint2d textSize = (VuoPoint2d){0,0};
		if(hasText)
		{
			textSize = VuoRenderedLayers_getTextSize(renderedLayers, label, labelFont, true, 1, 0, INFINITY, true);

			actualWidth  = fmax(actualWidth,  textSize.x);
			actualHeight = fmax(actualHeight, textSize.y);

			textLayer = (VuoLayer)VuoSceneText_make(label, f, true, INFINITY, labelPosition);
		}

		actualWidth  = fmax(actualWidth,  textSize.x + labelPadding.x * 2);
		actualHeight = fmax(actualHeight, textSize.y + labelPadding.y * 2);

		actualWidth  += borderThickness * 2;
		actualHeight += borderThickness * 2;

		if (VuoReal_areEqual(actualWidth, 0) || VuoReal_areEqual(actualHeight, 0))
		{
			VuoLayer_retain(textLayer);
			VuoLayer_release(textLayer);
			return nullptr;
		}

		float innerCornerRoundness = (actualHeight - borderThickness * 2 - (actualHeight * (1-cornerRoundness))) / (actualHeight - borderThickness * 2);

		VuoLayer backgroundLayer = VuoLayer_makeRoundedRectangle(	VuoText_make("Button Background"),
																	isPressed ? backgroundColorPressed : (isHovered ? backgroundColorHovered : backgroundColor),
																	VuoPoint2d_make(0,0),
																	0,
																	actualWidth  - borderThickness * 2,
																	actualHeight - borderThickness * 2,
																	1,
																	innerCornerRoundness);

		VuoLayer borderLayer = VuoLayer_makeRoundedRectangle(		VuoText_make("Button Border"),
																	isPressed ? borderColorPressed : (isHovered ? borderColorHovered : borderColor),
																	VuoPoint2d_make(0,0),
																	0,
																	actualWidth,
																	actualHeight,
																	1,
																	cornerRoundness);

		VuoPoint3d offset = VuoPoint3d_make(0,0,0);

		if (VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Left)
			offset.x = actualWidth * .5f;
		else if (VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Right)
			offset.x = -actualWidth * .5f;

		if (VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Top)
			offset.y = -actualHeight * .5f;
		else if (VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Bottom)
			offset.y = actualHeight * .5f;

		VuoSceneObject_translate((VuoSceneObject)backgroundLayer, offset);
		VuoSceneObject_translate((VuoSceneObject)borderLayer, offset);

		if(hasText)
		{
			VuoSceneObject_translate((VuoSceneObject)textLayer, offset);

			if (VuoAnchor_getHorizontal(labelPosition) != VuoHorizontalAlignment_Center)
				VuoSceneObject_translate((VuoSceneObject)textLayer, (VuoPoint3d){
					(VuoAnchor_getHorizontal(labelPosition) == VuoHorizontalAlignment_Left ? (-(actualWidth-labelPadding.x)) : (actualWidth-labelPadding.x)) * .5f,
					0, 0});

			if (VuoAnchor_getVertical(labelPosition) != VuoVerticalAlignment_Center)
				VuoSceneObject_translate((VuoSceneObject)textLayer, (VuoPoint3d){0,
					(VuoAnchor_getVertical(labelPosition) == VuoVerticalAlignment_Top ? (actualHeight-labelPadding.y) : (-(actualHeight-labelPadding.y))) * .5f,
					0});
		}

		VuoList_VuoLayer layers = VuoListCreate_VuoLayer();
		VuoLocal(layers);

		VuoListAppendValue_VuoLayer(layers, borderLayer);
		VuoListAppendValue_VuoLayer(layers, backgroundLayer);
		if(hasText) VuoListAppendValue_VuoLayer(layers, textLayer);

		return VuoLayer_makeGroup(layers, VuoTransform2d_make(position, 0, VuoPoint2d_make(1,1)));
	}
};

VuoSerializableRegister(VuoUiThemeButtonRounded);	///< Register with base class

/**
 * Creates a theme for button widgets, with the rounded style.
 */
VuoUiTheme VuoUiTheme_makeButtonRounded(VuoReal minimumWidth,
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
										VuoReal cornerRoundness)
{
	return reinterpret_cast<VuoUiTheme>(new VuoUiThemeButtonRounded(minimumWidth,
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
																	cornerRoundness));
}
