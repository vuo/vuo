/**
 * @file
 * Private VuoUiThemeToggleRounded implementation.
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
					  "title": "UI Theme: Toggle, Rounded",
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
 * A theme for toggle widgets, with the rounded style.
 */
class VuoUiThemeToggleRounded : public VuoUiThemeToggle
{
private:
	VuoFont labelFont;

	VuoColor labelColor;
	VuoColor labelColorHovered;
	VuoColor labelColorPressed;
	VuoColor labelColorToggled;
	VuoColor labelColorToggledAndHovered;

	VuoColor checkmarkColor;
	VuoColor checkmarkColorHovered;
	VuoColor checkmarkColorPressed;

	VuoColor checkmarkBorderColor;
	VuoColor checkmarkBorderColorHovered;
	VuoColor checkmarkBorderColorPressed;

	VuoColor checkboxBackgroundColor;
	VuoColor checkboxBackgroundColorHovered;
	VuoColor checkboxBackgroundColorPressed;
	VuoColor checkboxBackgroundColorToggled;
	VuoColor checkboxBackgroundColorToggledAndHovered;

	VuoColor checkboxBorderColor;
	VuoColor checkboxBorderColorHovered;
	VuoColor checkboxBorderColorPressed;
	VuoColor checkboxBorderColorToggled;
	VuoColor checkboxBorderColorToggledAndHovered;

	VuoReal checkboxBorderThickness;
	VuoReal checkboxCornerRoundness;
	VuoReal marginBetweenCheckboxAndLabel;

public:
	static std::string type; ///< The subtype's class name.

	/**
	 * Creates a rounded toggle theme from JSON.
	 */
	static VuoSerializable *makeFromJson(json_object *js)
	{
		return new VuoUiThemeToggleRounded(VuoJson_getObjectValue(VuoFont,    js, "labelFont",                                (VuoFont){VuoText_make("Avenir-Medium"), 24, false, (VuoColor){1,1,1,1}, VuoHorizontalAlignment_Center, 1, 1}),

										   VuoJson_getObjectValue(VuoColor,   js, "labelColor",                               (VuoColor){.7,.7,.7,.7}),
										   VuoJson_getObjectValue(VuoColor,   js, "labelColorHovered",                        (VuoColor){.7,.7,.7,.8}),
										   VuoJson_getObjectValue(VuoColor,   js, "labelColorPressed",                        (VuoColor){.7,.7,.7,.9}),
										   VuoJson_getObjectValue(VuoColor,   js, "labelColorToggled",                        (VuoColor){.7,.7,.7,.8}),
										   VuoJson_getObjectValue(VuoColor,   js, "labelColorToggledAndHovered",              (VuoColor){.7,.7,.7,.9}),

										   VuoJson_getObjectValue(VuoColor,   js, "checkmarkColor",                           (VuoColor){1,1,1,.9}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkmarkColorHovered",                    (VuoColor){1,1,1,1 }),
										   VuoJson_getObjectValue(VuoColor,   js, "checkmarkColorPressed",                    (VuoColor){1,1,1,.9}),

										   VuoJson_getObjectValue(VuoColor,   js, "checkmarkBorderColor",                     (VuoColor){0,0,0,.5}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkmarkBorderColorHovered",              (VuoColor){0,0,0,.5}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkmarkBorderColorPressed",              (VuoColor){0,0,0,1}),

										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBackgroundColor",                  (VuoColor){.4,.4, .4, 1}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBackgroundColorHovered",           (VuoColor){.43,.43,.43,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBackgroundColorPressed",           (VuoColor){.4,.5, .6, 1}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBackgroundColorToggled",           (VuoColor){.4,.6, .8, 1}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBackgroundColorToggledAndHovered", (VuoColor){.4,.62,.84,1}),

										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBorderColor",                      (VuoColor){.46,.46,.46,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBorderColorHovered",               (VuoColor){.46,.48,.49,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBorderColorPressed",               (VuoColor){.46,.55,.64,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBorderColorToggled",               (VuoColor){.46,.64,.82,1}),
										   VuoJson_getObjectValue(VuoColor,   js, "checkboxBorderColorToggledAndHovered",     (VuoColor){.46,.66,.86,1}),

										   VuoJson_getObjectValue(VuoReal,    js, "checkboxBorderThickness",                  0.005),
										   VuoJson_getObjectValue(VuoReal,    js, "checkboxCornerRoundness",                  0.5),
										   VuoJson_getObjectValue(VuoReal,    js, "marginBetweenCheckboxAndLabel",            0.025));
	}

	/**
	 * Creates a theme for toggle widgets, with the rounded style.
	 */
	VuoUiThemeToggleRounded(VuoFont _labelFont,

							VuoColor _labelColor,
							VuoColor _labelColorHovered,
							VuoColor _labelColorPressed,
							VuoColor _labelColorToggled,
							VuoColor _labelColorToggledAndHovered,

							VuoColor _checkmarkColor,
							VuoColor _checkmarkColorHovered,
							VuoColor _checkmarkColorPressed,

							VuoColor _checkmarkBorderColor,
							VuoColor _checkmarkBorderColorHovered,
							VuoColor _checkmarkBorderColorPressed,

							VuoColor _checkboxBackgroundColor,
							VuoColor _checkboxBackgroundColorHovered,
							VuoColor _checkboxBackgroundColorPressed,
							VuoColor _checkboxBackgroundColorToggled,
							VuoColor _checkboxBackgroundColorToggledAndHovered,

							VuoColor _checkboxBorderColor,
							VuoColor _checkboxBorderColorHovered,
							VuoColor _checkboxBorderColorPressed,
							VuoColor _checkboxBorderColorToggled,
							VuoColor _checkboxBorderColorToggledAndHovered,

							VuoReal _checkboxBorderThickness,
							VuoReal _checkboxCornerRoundness,
							VuoReal _marginBetweenCheckboxAndLabel)
	{
		labelFont = _labelFont;
		VuoFont_retain(labelFont);

		labelColor = _labelColor;
		labelColorHovered = _labelColorHovered;
		labelColorPressed = _labelColorPressed;
		labelColorToggled = _labelColorToggled;
		labelColorToggledAndHovered = _labelColorToggledAndHovered;

		checkmarkColor = _checkmarkColor;
		checkmarkColorHovered = _checkmarkColorHovered;
		checkmarkColorPressed = _checkmarkColorPressed;

		checkmarkBorderColor = _checkmarkBorderColor;
		checkmarkBorderColorHovered = _checkmarkBorderColorHovered;
		checkmarkBorderColorPressed = _checkmarkBorderColorPressed;

		checkboxBackgroundColor = _checkboxBackgroundColor;
		checkboxBackgroundColorHovered = _checkboxBackgroundColorHovered;
		checkboxBackgroundColorPressed = _checkboxBackgroundColorPressed;
		checkboxBackgroundColorToggled = _checkboxBackgroundColorToggled;
		checkboxBackgroundColorToggledAndHovered = _checkboxBackgroundColorToggledAndHovered;

		checkboxBorderColor = _checkboxBorderColor;
		checkboxBorderColorHovered = _checkboxBorderColorHovered;
		checkboxBorderColorPressed = _checkboxBorderColorPressed;
		checkboxBorderColorToggled = _checkboxBorderColorToggled;
		checkboxBorderColorToggledAndHovered = _checkboxBorderColorToggledAndHovered;

		checkboxBorderThickness = _checkboxBorderThickness;
		checkboxCornerRoundness = _checkboxCornerRoundness;
		marginBetweenCheckboxAndLabel = _marginBetweenCheckboxAndLabel;
	}
	~VuoUiThemeToggleRounded()
	{
		VuoFont_release(labelFont);
	}

	/**
	 * Encodes the theme as a JSON object.
	 */
	json_object *getJson()
	{
		json_object *json = VuoSerializable::getJson();

		json_object_object_add(json, "labelFont", VuoFont_getJson(labelFont));

		json_object_object_add(json, "labelColor",             VuoColor_getJson(labelColor));
		json_object_object_add(json, "labelColorHovered",      VuoColor_getJson(labelColorHovered));
		json_object_object_add(json, "labelColorPressed",      VuoColor_getJson(labelColorPressed));
		json_object_object_add(json, "labelColorToggled", VuoColor_getJson(labelColorToggled));
		json_object_object_add(json, "labelColorToggledAndHovered", VuoColor_getJson(labelColorToggledAndHovered));

		json_object_object_add(json, "checkmarkColor",        VuoColor_getJson(checkmarkColor));
		json_object_object_add(json, "checkmarkColorHovered", VuoColor_getJson(checkmarkColorHovered));
		json_object_object_add(json, "checkmarkColorPressed", VuoColor_getJson(checkmarkColorPressed));

		json_object_object_add(json, "checkmarkBorderColor",            VuoColor_getJson(checkmarkBorderColor));
		json_object_object_add(json, "checkmarkBorderColorHovered",     VuoColor_getJson(checkmarkBorderColorHovered));
		json_object_object_add(json, "checkmarkBorderColorPressed",     VuoColor_getJson(checkmarkBorderColorPressed));

		json_object_object_add(json, "checkboxBackgroundColor",        VuoColor_getJson(checkboxBackgroundColor));
		json_object_object_add(json, "checkboxBackgroundColorHovered", VuoColor_getJson(checkboxBackgroundColorHovered));
		json_object_object_add(json, "checkboxBackgroundColorPressed", VuoColor_getJson(checkboxBackgroundColorPressed));
		json_object_object_add(json, "checkboxBackgroundColorToggled", VuoColor_getJson(checkboxBackgroundColorToggled));
		json_object_object_add(json, "checkboxBackgroundColorToggledAndHovered", VuoColor_getJson(checkboxBackgroundColorToggledAndHovered));

		json_object_object_add(json, "checkboxBorderColor",            VuoColor_getJson(checkboxBorderColor));
		json_object_object_add(json, "checkboxBorderColorHovered",     VuoColor_getJson(checkboxBorderColorHovered));
		json_object_object_add(json, "checkboxBorderColorPressed",     VuoColor_getJson(checkboxBorderColorPressed));
		json_object_object_add(json, "checkboxBorderColorToggled",     VuoColor_getJson(checkboxBorderColorToggled));
		json_object_object_add(json, "checkboxBorderColorToggledAndHovered",     VuoColor_getJson(checkboxBorderColorToggledAndHovered));

		json_object_object_add(json, "checkboxBorderThickness", VuoReal_getJson(checkboxBorderThickness));
		json_object_object_add(json, "checkboxCornerRoundness", VuoReal_getJson(checkboxCornerRoundness));
		json_object_object_add(json, "marginBetweenCheckboxAndLabel", VuoReal_getJson(marginBetweenCheckboxAndLabel));
		return json;
	}

	/**
	 * Returns a compact string representation of the theme.
	 */
	char *getSummary()
	{
		return strdup("Toggle Button Theme (Rounded)");
	}

	/**
	 * Returns true if both themes are of the same subtype, and their values are equal.
	 */
	bool operator==(const VuoSerializable &that)
	{
		VuoSerializableEquals(VuoUiThemeToggleRounded);
		return VuoFont_areEqual(labelFont,                       thatSpecialized->labelFont)

			&& VuoColor_areEqual(labelColor,                     thatSpecialized->labelColor)
			&& VuoColor_areEqual(labelColorHovered,              thatSpecialized->labelColorHovered)
			&& VuoColor_areEqual(labelColorPressed,              thatSpecialized->labelColorPressed)
			&& VuoColor_areEqual(labelColorToggled,              thatSpecialized->labelColorToggled)
			&& VuoColor_areEqual(labelColorToggledAndHovered,              thatSpecialized->labelColorToggledAndHovered)

			&& VuoColor_areEqual(checkmarkColor,                 thatSpecialized->checkmarkColor)
			&& VuoColor_areEqual(checkmarkColorHovered,          thatSpecialized->checkmarkColorHovered)
			&& VuoColor_areEqual(checkmarkColorPressed,          thatSpecialized->checkmarkColorPressed)

			&& VuoColor_areEqual(checkmarkBorderColor,           thatSpecialized->checkmarkBorderColor)
			&& VuoColor_areEqual(checkmarkBorderColorHovered,    thatSpecialized->checkmarkBorderColorHovered)
			&& VuoColor_areEqual(checkmarkBorderColorPressed,    thatSpecialized->checkmarkBorderColorPressed)

			&& VuoColor_areEqual(checkboxBackgroundColor,        thatSpecialized->checkboxBackgroundColor)
			&& VuoColor_areEqual(checkboxBackgroundColorHovered, thatSpecialized->checkboxBackgroundColorHovered)
			&& VuoColor_areEqual(checkboxBackgroundColorPressed, thatSpecialized->checkboxBackgroundColorPressed)
			&& VuoColor_areEqual(checkboxBackgroundColorToggled, thatSpecialized->checkboxBackgroundColorToggled)
			&& VuoColor_areEqual(checkboxBackgroundColorToggledAndHovered, thatSpecialized->checkboxBackgroundColorToggledAndHovered)

			&& VuoColor_areEqual(checkboxBorderColor,            thatSpecialized->checkboxBorderColor)
			&& VuoColor_areEqual(checkboxBorderColorHovered,     thatSpecialized->checkboxBorderColorHovered)
			&& VuoColor_areEqual(checkboxBorderColorPressed,     thatSpecialized->checkboxBorderColorPressed)
			&& VuoColor_areEqual(checkboxBorderColorToggled,     thatSpecialized->checkboxBorderColorToggled)
			&& VuoColor_areEqual(checkboxBorderColorToggledAndHovered,     thatSpecialized->checkboxBorderColorToggledAndHovered)

			&& VuoReal_areEqual(checkboxBorderThickness,         thatSpecialized->checkboxBorderThickness)
			&& VuoReal_areEqual(checkboxCornerRoundness,         thatSpecialized->checkboxCornerRoundness)
			&& VuoReal_areEqual(marginBetweenCheckboxAndLabel,   thatSpecialized->marginBetweenCheckboxAndLabel);
	}

	/**
	 * Returns true if this theme sorts before `that` theme.
	 */
	bool operator<(const VuoSerializable &that)
	{
		VuoSerializableLessThan(VuoUiThemeToggleRounded);

		VuoType_returnInequality(VuoFont,    labelFont,                      thatSpecialized->labelFont);

		VuoType_returnInequality(VuoColor,   labelColor,                     thatSpecialized->labelColor);
		VuoType_returnInequality(VuoColor,   labelColorHovered,              thatSpecialized->labelColorHovered);
		VuoType_returnInequality(VuoColor,   labelColorPressed,              thatSpecialized->labelColorPressed);
		VuoType_returnInequality(VuoColor,   labelColorToggled,              thatSpecialized->labelColorToggled);
		VuoType_returnInequality(VuoColor,   labelColorToggledAndHovered,              thatSpecialized->labelColorToggledAndHovered);

		VuoType_returnInequality(VuoColor,   checkmarkColor,                 thatSpecialized->checkmarkColor);
		VuoType_returnInequality(VuoColor,   checkmarkColorHovered,          thatSpecialized->checkmarkColorHovered);
		VuoType_returnInequality(VuoColor,   checkmarkColorPressed,          thatSpecialized->checkmarkColorPressed);

		VuoType_returnInequality(VuoColor,   checkmarkBorderColor,           thatSpecialized->checkmarkBorderColor);
		VuoType_returnInequality(VuoColor,   checkmarkBorderColorHovered,    thatSpecialized->checkmarkBorderColorHovered);
		VuoType_returnInequality(VuoColor,   checkmarkBorderColorPressed,    thatSpecialized->checkmarkBorderColorPressed);

		VuoType_returnInequality(VuoColor,   checkboxBackgroundColor,        thatSpecialized->checkboxBackgroundColor);
		VuoType_returnInequality(VuoColor,   checkboxBackgroundColorHovered, thatSpecialized->checkboxBackgroundColorHovered);
		VuoType_returnInequality(VuoColor,   checkboxBackgroundColorPressed, thatSpecialized->checkboxBackgroundColorPressed);
		VuoType_returnInequality(VuoColor,   checkboxBackgroundColorToggled, thatSpecialized->checkboxBackgroundColorToggled);
		VuoType_returnInequality(VuoColor,   checkboxBackgroundColorToggledAndHovered, thatSpecialized->checkboxBackgroundColorToggledAndHovered);

		VuoType_returnInequality(VuoColor,   checkboxBorderColor,            thatSpecialized->checkboxBorderColor);
		VuoType_returnInequality(VuoColor,   checkboxBorderColorHovered,     thatSpecialized->checkboxBorderColorHovered);
		VuoType_returnInequality(VuoColor,   checkboxBorderColorPressed,     thatSpecialized->checkboxBorderColorPressed);
		VuoType_returnInequality(VuoColor,   checkboxBorderColorToggled,     thatSpecialized->checkboxBorderColorToggled);
		VuoType_returnInequality(VuoColor,   checkboxBorderColorToggledAndHovered,     thatSpecialized->checkboxBorderColorToggledAndHovered);

		VuoType_returnInequality(VuoReal,    checkboxBorderThickness,        thatSpecialized->checkboxBorderThickness);
		VuoType_returnInequality(VuoReal,    checkboxCornerRoundness,        thatSpecialized->checkboxCornerRoundness);
		VuoType_returnInequality(VuoReal,    marginBetweenCheckboxAndLabel,  thatSpecialized->marginBetweenCheckboxAndLabel);

		return false;
	}

	/**
	 * Creates a layer group representing a toggle with the specified theme and parameters.
	 *
	 *    - borderLayer — the outermost edge of the checkbox
	 *    - backgroundLayer — inset from borderLayer
	 *    - toggleLayer (optional) — checkmark
	 *    - textLayer (optional) — label
	 */
	VuoLayer render(VuoRenderedLayers renderedLayers, VuoText label, VuoPoint2d position, VuoAnchor anchor, bool isHovered, bool isPressed, bool isToggled)
	{
		bool hasText = VuoText_length(label) > 0;

		VuoReal lineHeight = VuoImageText_getLineHeight(labelFont, VuoGraphicsWindowDefaultWidth, 1);
		float checkboxWidth = lineHeight * .6;
		float checkboxHeight = checkboxWidth;

		checkboxWidth  += checkboxBorderThickness * 2;
		checkboxHeight += checkboxBorderThickness * 2;

		float innerCornerRoundness = (checkboxHeight - checkboxBorderThickness * 2 - (checkboxHeight * (1 - checkboxCornerRoundness))) / (checkboxHeight - checkboxBorderThickness * 2);

		VuoLayer backgroundLayer = VuoLayer_makeRoundedRectangle(VuoText_make("Checkbox Background"),
																 isPressed ? checkboxBackgroundColorPressed :
																(isToggled && isHovered ? checkboxBackgroundColorToggledAndHovered :
																(isToggled ? checkboxBackgroundColorToggled :
																(isHovered ? checkboxBackgroundColorHovered :
																             checkboxBackgroundColor))),
																 VuoPoint2d_make(0, 0),
																 0,
																 checkboxWidth  - checkboxBorderThickness * 2,
																 checkboxHeight - checkboxBorderThickness * 2,
																 1,
																 innerCornerRoundness);


		VuoPoint2d checkmarkPosition{ (float)checkboxWidth * .15f, (float)checkboxHeight * .15f};
		VuoReal checkmarkWidth = checkboxWidth * 1.15;

		// Ensures the layer bounds don't change when the checkmark appears or disappears.
		// Also covers the gap between the checkbox and the label.
		VuoLayer spacerLayer = VuoLayer_makeColor(VuoText_make("Spacer"),
												  (VuoColor){ 0, 0, 0, 0 },
												  VuoPoint2d_add(checkmarkPosition, (VuoPoint2d){ (float)marginBetweenCheckboxAndLabel / 2, 0 }),
												  0,
												  checkmarkWidth + marginBetweenCheckboxAndLabel,
												  checkmarkWidth);

		VuoLayer toggleLayer = nullptr;
		if (isToggled)
			toggleLayer = VuoLayer_makeCheckmark(VuoText_make("Checkmark"),

												 isPressed ? checkmarkColorPressed :
												(isHovered ? checkmarkColorHovered :
															 checkmarkColor),

												 isPressed ? checkmarkBorderColorPressed :
												(isHovered ? checkmarkBorderColorHovered :
															 checkmarkBorderColor),

												 .01,
												 checkmarkPosition,
												 0,
												 checkmarkWidth,
												 checkmarkWidth);

		VuoLayer borderLayer = VuoLayer_makeRoundedRectangle(VuoText_make("Checkbox Border"),
															 isPressed ? checkboxBorderColorPressed :
															(isToggled && isHovered ? checkboxBorderColorToggledAndHovered :
															(isToggled ? checkboxBorderColorToggled :
															(isHovered ? checkboxBorderColorHovered :
															             checkboxBorderColor))),
															 VuoPoint2d_make(0, 0),
															 0,
															 checkboxWidth,
															 checkboxHeight,
															 1,
															 checkboxCornerRoundness);

		VuoList_VuoLayer layers = VuoListCreate_VuoLayer();
		VuoLocal(layers);

		VuoListAppendValue_VuoLayer(layers, spacerLayer);
		VuoListAppendValue_VuoLayer(layers, borderLayer);
		VuoListAppendValue_VuoLayer(layers, backgroundLayer);
		if(VuoLayer_isPopulated(toggleLayer)) VuoListAppendValue_VuoLayer(layers, toggleLayer);

		if (hasText)
		{
			VuoFont f = labelFont;
			if (isPressed)
				f.color = (VuoColor){ f.color.r * labelColorPressed.r,
									  f.color.g * labelColorPressed.g,
									  f.color.b * labelColorPressed.b,
									  f.color.a * labelColorPressed.a };
			else if (isToggled && isHovered)
				f.color = (VuoColor){ f.color.r * labelColorToggledAndHovered.r,
									  f.color.g * labelColorToggledAndHovered.g,
									  f.color.b * labelColorToggledAndHovered.b,
									  f.color.a * labelColorToggledAndHovered.a };
			else if (isHovered)
				f.color = (VuoColor){ f.color.r * labelColorHovered.r,
									  f.color.g * labelColorHovered.g,
									  f.color.b * labelColorHovered.b,
									  f.color.a * labelColorHovered.a };
			else if (isToggled)
				f.color = (VuoColor){ f.color.r * labelColorToggled.r,
									  f.color.g * labelColorToggled.g,
									  f.color.b * labelColorToggled.b,
									  f.color.a * labelColorToggled.a };
			else
				f.color = (VuoColor){ f.color.r * labelColor.r,
									  f.color.g * labelColor.g,
									  f.color.b * labelColor.b,
									  f.color.a * labelColor.a };

			VuoSceneObject textLayer = VuoSceneText_make(label, f, true, INFINITY, VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Center));
			VuoSceneObject_setTransform(textLayer, VuoTransform_makeEuler((VuoPoint3d){ (float)checkboxWidth/2 + (float)marginBetweenCheckboxAndLabel, -.0001f * (float)f.pointSize, 0 },
																					  (VuoPoint3d){ 0, 0, 0 },
																					  (VuoPoint3d){ 1, 1, 1 }));
			VuoListAppendValue_VuoLayer(layers, (VuoLayer)textLayer);
		}


		VuoLayer group = VuoLayer_makeGroup(layers, VuoTransform2d_make(position, 0, VuoPoint2d_make(1, 1)));

		// Center horizontally.
		VuoPoint2d textSize = VuoRenderedLayers_getTextSize(renderedLayers, label, labelFont, true, 1, 0, INFINITY, true);
		VuoSceneObject_translate((VuoSceneObject)group, (VuoPoint3d){checkboxWidth/2 - (checkboxWidth + (float)marginBetweenCheckboxAndLabel + textSize.x)/2, 0, 0});

		unsigned long int pixelsWide, pixelsHigh;
		float backingScaleFactor;
		if (!VuoRenderedLayers_getRenderingDimensions(renderedLayers, &pixelsWide, &pixelsHigh, &backingScaleFactor))
		{
			VuoRetain(group);
			VuoRelease(group);
			return nullptr;
		}

		// Wrap it in an extra group, so the above centering and the anchor aren't disturbed by e.g. `Arrange Layers in Column`.
		VuoList_VuoLayer l = VuoListCreate_VuoLayer();
		VuoRetain(l);
		VuoListAppendValue_VuoLayer(l, VuoLayer_setAnchor(group, anchor, pixelsWide, pixelsHigh, backingScaleFactor));
		VuoLayer lg = VuoLayer_makeGroup(l, VuoTransform2d_makeIdentity());
		VuoRelease(l);
		return lg;
	}
};

VuoSerializableRegister(VuoUiThemeToggleRounded);  ///< Register with base class

/**
 * Creates a theme for toggle widgets, with the rounded style.
 */
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
										VuoReal marginBetweenCheckboxAndLabel)
{
	return reinterpret_cast<VuoUiTheme>(new VuoUiThemeToggleRounded(labelFont,

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
																	marginBetweenCheckboxAndLabel));
}
