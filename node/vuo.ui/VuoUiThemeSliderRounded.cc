/**
 * @file
 * Private VuoUiThemeSliderRounded implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoUiThemeBase.hh"

#include "type.h"

extern "C" {
#include "VuoImage.h"
#include "VuoAnchor.h"
#include "VuoPoint2d.h"
#include "VuoSceneText.h"
#include "VuoShader.h"
}

#include "VuoUiTheme.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title": "UI Theme: Slider (Rounded)",
					  "dependencies": [
						  "VuoBoolean",
						  "VuoColor",
						  "VuoFont",
						  "VuoImage",
						  "VuoLayer",
						  "VuoPoint2d",
						  "VuoReal",
						  "VuoRectangle",
						  "VuoSceneText",
						  "VuoAnchor",
						  "VuoHorizontalAlignment",
						  "VuoVerticalAlignment"
					  ],
				  });
#endif
/// @}

/**
 * A theme for slider widgets, with the rounded style.
 */
class VuoUiThemeSliderRounded : public VuoUiThemeSlider
{
private:
	VuoFont labelFont;
	VuoColor labelColor;
	VuoColor labelColorHovered;

	VuoReal handleWidth;
	VuoReal handleHeight;
	VuoReal handleBorderThickness;
	VuoReal handleCornerRoundness;

	VuoColor handleColor;
	VuoColor handleColorHovered;
	VuoColor handleColorPressed;

	VuoColor handleBorderColor;
	VuoColor handleBorderColorHovered;
	VuoColor handleBorderColorPressed;

	VuoReal trackDepth;
	VuoReal trackBorderThickness;
	VuoReal trackCornerRoundness;

	VuoColor activeTrackColor;
	VuoColor activeTrackColorHovered;

	VuoColor activeTrackBorderColor;
	VuoColor activeTrackBorderColorHovered;

	VuoColor inactiveTrackColor;
	VuoColor inactiveTrackColorHovered;

	VuoColor inactiveTrackBorderColor;
	VuoColor inactiveTrackBorderColorHovered;

	VuoReal marginBetweenTrackAndLabel;

public:
	static std::string type; ///< The subtype's class name.

	/**
	 * Creates a rounded slider theme from JSON.
	 */
	static VuoSerializable *makeFromJson(json_object *js)
	{
		return new VuoUiThemeSliderRounded(
			VuoJson_getObjectValue(VuoFont,    js, "labelFont",                       (VuoFont){VuoText_make("Avenir-Medium"), 24, false, (VuoColor){1,1,1,1}, VuoHorizontalAlignment_Center, 1, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "labelColor",                      (VuoColor){.7,.7,.7,.7}),
			VuoJson_getObjectValue(VuoColor,   js, "labelColorHovered",               (VuoColor){.7,.7,.7,.8}),

			VuoJson_getObjectValue(VuoReal,    js, "handleWidth",                     .06),
			VuoJson_getObjectValue(VuoReal,    js, "handleHeight",                    .06),
			VuoJson_getObjectValue(VuoReal,    js, "handleBorderThickness",           .005),
			VuoJson_getObjectValue(VuoReal,    js, "handleCornerRoundness",           1),

			VuoJson_getObjectValue(VuoColor,   js, "handleColor",                     (VuoColor){.4,.5, .6, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "handleColorHovered",              (VuoColor){.4,.52,.64,1}),
			VuoJson_getObjectValue(VuoColor,   js, "handleColorPressed",              (VuoColor){.4,.6, .8, 1}),

			VuoJson_getObjectValue(VuoColor,   js, "handleBorderColor",               (VuoColor){.2,.3, .4, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "handleBorderColorHovered",        (VuoColor){.2,.32,.44,1}),
			VuoJson_getObjectValue(VuoColor,   js, "handleBorderColorPressed",        (VuoColor){.2,.4, .6, 1}),

			VuoJson_getObjectValue(VuoReal,    js, "trackDepth",                      .015),
			VuoJson_getObjectValue(VuoReal,    js, "trackBorderThickness",            .005),
			VuoJson_getObjectValue(VuoReal,    js, "trackCornerRoundness",            1),

			VuoJson_getObjectValue(VuoColor,   js, "activeTrackColor",                (VuoColor){.4,.5, .6, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "activeTrackColorHovered",         (VuoColor){.4,.52,.64,1}),

			VuoJson_getObjectValue(VuoColor,   js, "activeTrackBorderColor",          (VuoColor){.2,.3, .4, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "activeTrackBorderColorHovered",   (VuoColor){.2,.32,.44,1}),

			VuoJson_getObjectValue(VuoColor,   js, "inactiveTrackColor",              (VuoColor){.4,.4, .4, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "inactiveTrackColorHovered",       (VuoColor){.4,.42,.44,1}),

			VuoJson_getObjectValue(VuoColor,   js, "inactiveTrackBorderColor",        (VuoColor){.2,.2, .2, .9}),
			VuoJson_getObjectValue(VuoColor,   js, "inactiveTrackBorderColorHovered", (VuoColor){.24,.24,.24,1}),

			VuoJson_getObjectValue(VuoReal,    js, "marginBetweenTrackAndLabel",      0.01));
	}

	/**
	 * Creates a theme for slider widgets, with the rounded style.
	 */
	VuoUiThemeSliderRounded(
		VuoFont _labelFont,
		VuoColor _labelColor,
		VuoColor _labelColorHovered,

		VuoReal _handleWidth,
		VuoReal _handleHeight,
		VuoReal _handleBorderThickness,
		VuoReal _handleCornerRoundness,

		VuoColor _handleColor,
		VuoColor _handleColorHovered,
		VuoColor _handleColorPressed,

		VuoColor _handleBorderColor,
		VuoColor _handleBorderColorHovered,
		VuoColor _handleBorderColorPressed,

		VuoReal _trackDepth,
		VuoReal _trackBorderThickness,
		VuoReal _trackCornerRoundness,

		VuoColor _activeTrackColor,
		VuoColor _activeTrackColorHovered,

		VuoColor _activeTrackBorderColor,
		VuoColor _activeTrackBorderColorHovered,

		VuoColor _inactiveTrackColor,
		VuoColor _inactiveTrackColorHovered,

		VuoColor _inactiveTrackBorderColor,
		VuoColor _inactiveTrackBorderColorHovered,

		VuoReal _marginBetweenTrackAndLabel)
	{
		labelFont = _labelFont;
		VuoFont_retain(labelFont);

		labelColor = _labelColor;
		labelColorHovered = _labelColorHovered;

		handleWidth = _handleWidth;
		handleHeight = _handleHeight;
		handleBorderThickness = _handleBorderThickness;
		handleCornerRoundness = _handleCornerRoundness;

		handleColor = _handleColor;
		handleColorHovered = _handleColorHovered;
		handleColorPressed = _handleColorPressed;

		handleBorderColor = _handleBorderColor;
		handleBorderColorHovered = _handleBorderColorHovered;
		handleBorderColorPressed = _handleBorderColorPressed;

		trackDepth = _trackDepth;
		trackBorderThickness = _trackBorderThickness;
		trackCornerRoundness = _trackCornerRoundness;

		activeTrackColor = _activeTrackColor;
		activeTrackColorHovered = _activeTrackColorHovered;

		activeTrackBorderColor = _activeTrackBorderColor;
		activeTrackBorderColorHovered = _activeTrackBorderColorHovered;

		inactiveTrackColor = _inactiveTrackColor;
		inactiveTrackColorHovered = _inactiveTrackColorHovered;

		inactiveTrackBorderColor = _inactiveTrackBorderColor;
		inactiveTrackBorderColorHovered = _inactiveTrackBorderColorHovered;

		marginBetweenTrackAndLabel = _marginBetweenTrackAndLabel;
	}

	~VuoUiThemeSliderRounded()
	{
		VuoFont_release(labelFont);
	}

	/**
	 * Encodes the theme as a JSON object.
	 */
	json_object *getJson()
	{
		json_object *js = VuoSerializable::getJson();

		json_object_object_add(js, "labelFont", VuoFont_getJson(labelFont));
		json_object_object_add(js, "labelColor", VuoColor_getJson(labelColor));
		json_object_object_add(js, "labelColorHovered", VuoColor_getJson(labelColorHovered));

		json_object_object_add(js, "handleWidth", VuoReal_getJson(handleWidth));
		json_object_object_add(js, "handleHeight", VuoReal_getJson(handleHeight));
		json_object_object_add(js, "handleBorderThickness", VuoReal_getJson(handleBorderThickness));
		json_object_object_add(js, "handleCornerRoundness", VuoReal_getJson(handleCornerRoundness));

		json_object_object_add(js, "handleColor", VuoColor_getJson(handleColor));
		json_object_object_add(js, "handleColorHovered", VuoColor_getJson(handleColorHovered));
		json_object_object_add(js, "handleColorPressed", VuoColor_getJson(handleColorPressed));

		json_object_object_add(js, "handleBorderColor", VuoColor_getJson(handleBorderColor));
		json_object_object_add(js, "handleBorderColorHovered", VuoColor_getJson(handleBorderColorHovered));
		json_object_object_add(js, "handleBorderColorPressed", VuoColor_getJson(handleBorderColorPressed));

		json_object_object_add(js, "trackDepth", VuoReal_getJson(trackDepth));
		json_object_object_add(js, "trackBorderThickness", VuoReal_getJson(trackBorderThickness));
		json_object_object_add(js, "trackCornerRoundness", VuoReal_getJson(trackCornerRoundness));

		json_object_object_add(js, "activeTrackColor", VuoColor_getJson(activeTrackColor));
		json_object_object_add(js, "activeTrackColorHovered", VuoColor_getJson(activeTrackColorHovered));

		json_object_object_add(js, "activeTrackBorderColor", VuoColor_getJson(activeTrackBorderColor));
		json_object_object_add(js, "activeTrackBorderColorHovered", VuoColor_getJson(activeTrackBorderColorHovered));

		json_object_object_add(js, "inactiveTrackColor", VuoColor_getJson(inactiveTrackColor));
		json_object_object_add(js, "inactiveTrackColorHovered", VuoColor_getJson(inactiveTrackColorHovered));

		json_object_object_add(js, "inactiveTrackBorderColor", VuoColor_getJson(inactiveTrackBorderColor));
		json_object_object_add(js, "inactiveTrackBorderColorHovered", VuoColor_getJson(inactiveTrackBorderColorHovered));

		json_object_object_add(js, "marginBetweenTrackAndLabel", VuoReal_getJson(marginBetweenTrackAndLabel));

		return js;
	}

	/**
	 * Returns a compact string representation of the theme.
	 */
	char *getSummary()
	{
		return strdup("Slider Theme (Rounded)");
	}

	/**
	 * Returns true if both themes are of the same subtype, and their values are equal.
	 */
	bool operator==(const VuoSerializable &that)
	{
		VuoSerializableEquals(VuoUiThemeSliderRounded);

		return VuoFont_areEqual(labelFont, thatSpecialized->labelFont)
			&& VuoColor_areEqual(labelColor, thatSpecialized->labelColor)
			&& VuoColor_areEqual(labelColorHovered, thatSpecialized->labelColorHovered)

			&& VuoReal_areEqual(handleWidth, thatSpecialized->handleWidth)
			&& VuoReal_areEqual(handleHeight, thatSpecialized->handleHeight)
			&& VuoReal_areEqual(handleBorderThickness, thatSpecialized->handleBorderThickness)
			&& VuoReal_areEqual(handleCornerRoundness, thatSpecialized->handleCornerRoundness)

			&& VuoColor_areEqual(handleColor, thatSpecialized->handleColor)
			&& VuoColor_areEqual(handleColorHovered, thatSpecialized->handleColorHovered)
			&& VuoColor_areEqual(handleColorPressed, thatSpecialized->handleColorPressed)

			&& VuoColor_areEqual(handleBorderColor, thatSpecialized->handleBorderColor)
			&& VuoColor_areEqual(handleBorderColorHovered, thatSpecialized->handleBorderColorHovered)
			&& VuoColor_areEqual(handleBorderColorPressed, thatSpecialized->handleBorderColorPressed)

			&& VuoReal_areEqual(trackDepth, thatSpecialized->trackDepth)
			&& VuoReal_areEqual(trackBorderThickness, thatSpecialized->trackBorderThickness)
			&& VuoReal_areEqual(trackCornerRoundness, thatSpecialized->trackCornerRoundness)

			&& VuoColor_areEqual(activeTrackColor, thatSpecialized->activeTrackColor)
			&& VuoColor_areEqual(activeTrackColorHovered, thatSpecialized->activeTrackColorHovered)

			&& VuoColor_areEqual(activeTrackBorderColor, thatSpecialized->activeTrackBorderColor)
			&& VuoColor_areEqual(activeTrackBorderColorHovered, thatSpecialized->activeTrackBorderColorHovered)

			&& VuoColor_areEqual(inactiveTrackColor, thatSpecialized->inactiveTrackColor)
			&& VuoColor_areEqual(inactiveTrackColorHovered, thatSpecialized->inactiveTrackColorHovered)

			&& VuoColor_areEqual(inactiveTrackBorderColor, thatSpecialized->inactiveTrackBorderColor)
			&& VuoColor_areEqual(inactiveTrackBorderColorHovered, thatSpecialized->inactiveTrackBorderColorHovered)

			&& VuoReal_areEqual(marginBetweenTrackAndLabel, thatSpecialized->marginBetweenTrackAndLabel);
	}

	/**
	 * Returns true if this theme sorts before `that` theme.
	 */
	bool operator<(const VuoSerializable &that)
	{
		VuoSerializableLessThan(VuoUiThemeSliderRounded);

		VuoType_returnInequality(VuoFont, labelFont, thatSpecialized->labelFont);
		VuoType_returnInequality(VuoColor, labelColor, thatSpecialized->labelColor);
		VuoType_returnInequality(VuoColor, labelColorHovered, thatSpecialized->labelColorHovered);

		VuoType_returnInequality(VuoReal, handleWidth, thatSpecialized->handleWidth);
		VuoType_returnInequality(VuoReal, handleHeight, thatSpecialized->handleHeight);
		VuoType_returnInequality(VuoReal, handleBorderThickness, thatSpecialized->handleBorderThickness);
		VuoType_returnInequality(VuoReal, handleCornerRoundness, thatSpecialized->handleCornerRoundness);

		VuoType_returnInequality(VuoColor, handleColor, thatSpecialized->handleColor);
		VuoType_returnInequality(VuoColor, handleColorHovered, thatSpecialized->handleColorHovered);
		VuoType_returnInequality(VuoColor, handleColorPressed, thatSpecialized->handleColorPressed);

		VuoType_returnInequality(VuoColor, handleBorderColor, thatSpecialized->handleBorderColor);
		VuoType_returnInequality(VuoColor, handleBorderColorHovered, thatSpecialized->handleBorderColorHovered);
		VuoType_returnInequality(VuoColor, handleBorderColorPressed, thatSpecialized->handleBorderColorPressed);

		VuoType_returnInequality(VuoReal, trackDepth, thatSpecialized->trackDepth);
		VuoType_returnInequality(VuoReal, trackBorderThickness, thatSpecialized->trackBorderThickness);
		VuoType_returnInequality(VuoReal, trackCornerRoundness, thatSpecialized->trackCornerRoundness);

		VuoType_returnInequality(VuoColor, activeTrackColor, thatSpecialized->activeTrackColor);
		VuoType_returnInequality(VuoColor, activeTrackColorHovered, thatSpecialized->activeTrackColorHovered);

		VuoType_returnInequality(VuoColor, activeTrackBorderColor, thatSpecialized->activeTrackBorderColor);
		VuoType_returnInequality(VuoColor, activeTrackBorderColorHovered, thatSpecialized->activeTrackBorderColorHovered);

		VuoType_returnInequality(VuoColor, inactiveTrackColor, thatSpecialized->inactiveTrackColor);
		VuoType_returnInequality(VuoColor, inactiveTrackColorHovered, thatSpecialized->inactiveTrackColorHovered);

		VuoType_returnInequality(VuoColor, inactiveTrackBorderColor, thatSpecialized->inactiveTrackBorderColor);
		VuoType_returnInequality(VuoColor, inactiveTrackBorderColorHovered, thatSpecialized->inactiveTrackBorderColorHovered);

		VuoType_returnInequality(VuoReal, marginBetweenTrackAndLabel, thatSpecialized->marginBetweenTrackAndLabel);

		return false;
	}

	/**
	 * Create a rounded rectangle layer with a progress bar.
	 */
	VuoLayer makeRoundedRectangleTrack(
		VuoText name,
		VuoColor background,
		VuoColor active,
		VuoPoint2d center,
		VuoReal rotation,
		VuoReal width,
		VuoReal height,
		VuoReal sharpness,
		VuoReal roundness,
		VuoOrientation orientation,
		VuoReal value)
	{
		// Since VuoShader_makeUnlitRoundedRectangleShader() produces a shader that fills half the size (to leave enough room for sharpness=0),
		// make the layer twice the specified size.
		VuoSceneObject so = VuoSceneObject_makeQuad(
					VuoShader_makeUnlitRoundedRectangleTrackShader(
						background,
						active,
						sharpness,
						roundness,
						width/height,
						orientation == VuoOrientation_Horizontal,
						value),
					VuoPoint3d_make(center.x, center.y, 0),
					VuoPoint3d_make(0, 0, rotation),
					width  * 2,
					height * 2 );
		VuoSceneObject_setName(so, name);
		return (VuoLayer)so;
	}

	/**
	 * Creates a layer tree representing a slider track with the specified theme and parameters.
	 */
	VuoLayer render(VuoRenderedLayers renderedLayers,
					VuoText label,
					VuoReal trackLength,
					VuoReal normalizedProgress,
					VuoPoint2d position,
					VuoAnchor anchor,
					VuoOrientation orientation,
					bool isHovered,
					bool isPressed)
	{
		VuoList_VuoLayer layers = VuoListCreate_VuoLayer();
		VuoLocal(layers);

		// Track.
		{
			float width  = orientation == VuoOrientation_Horizontal ? trackLength : trackDepth;
			float height = orientation == VuoOrientation_Horizontal ? trackDepth : trackLength;

			float outerTrackWidth  = width  + trackBorderThickness * 2;
			float outerTrackHeight = height + trackBorderThickness * 2;
			float innerTrackCornerRoundness = (outerTrackHeight - trackBorderThickness * 2 - (outerTrackHeight * (1 - trackCornerRoundness))) / (outerTrackHeight - trackBorderThickness * 2);

			float hml           = handleMovementLength(trackLength);
			float hhml          = trackLength - (trackLength - hml)/2;
			float innerProgress = (trackLength - hhml)/trackLength + normalizedProgress * (trackLength - (trackLength - hhml) * 2) / trackLength;
			float outerProgress = (trackBorderThickness + innerProgress * trackLength) / (trackLength + trackBorderThickness * 2);

			VuoLayer trackLayer = makeRoundedRectangleTrack(VuoText_make("Slider Track"),
															isHovered ? inactiveTrackColorHovered : inactiveTrackColor,
															isHovered ? activeTrackColorHovered : activeTrackColor,
															VuoPoint2d_make(0, 0),
															0,
															width,
															height,
															1,
															innerTrackCornerRoundness,
															orientation,
															innerProgress);

			VuoLayer trackBorderLayer = makeRoundedRectangleTrack(VuoText_make("Slider Track Border"),
																  isHovered ? inactiveTrackBorderColorHovered : inactiveTrackBorderColor,
																  isHovered ? activeTrackBorderColorHovered : activeTrackBorderColor,
																  VuoPoint2d_make(0, 0),
																  0,
																  outerTrackWidth,
																  outerTrackHeight,
																  1,
																  trackCornerRoundness,
																  orientation,
																  outerProgress);

			VuoListAppendValue_VuoLayer(layers, trackBorderLayer);
			VuoListAppendValue_VuoLayer(layers, trackLayer);
		}


		// Handle.
		{
			float width  = orientation == VuoOrientation_Horizontal ? handleWidth : handleHeight;
			float height = orientation == VuoOrientation_Horizontal ? handleHeight : handleWidth;

			float hml = handleMovementLength(trackLength);
			float handlePos = VuoReal_lerp(-hml / 2, hml / 2, normalizedProgress);
			VuoPoint2d handlePosition = (VuoPoint2d){
				orientation == VuoOrientation_Horizontal ? handlePos : 0,
				orientation == VuoOrientation_Horizontal ? 0 : handlePos,
			};

			float outerHandleWidth  = width  + handleBorderThickness * 2;
			float outerHandleHeight = height + handleBorderThickness * 2;
			float innerHandleCornerRoundness = (outerHandleHeight - handleBorderThickness * 2 - (outerHandleHeight * (1 - handleCornerRoundness))) / (outerHandleHeight - handleBorderThickness * 2);

			VuoLayer handleLayer = VuoLayer_makeRoundedRectangle(VuoText_make("Slider Handle"),
																 isPressed ? handleColorPressed : (isHovered ? handleColorHovered : handleColor),
																 handlePosition,
																 0,
																 width,
																 height,
																 1,
																 innerHandleCornerRoundness);

			VuoLayer handleBorderLayer = VuoLayer_makeRoundedRectangle(VuoText_make("Slider Handle Border"),
																	   isPressed ? handleBorderColorPressed : (isHovered ? handleBorderColorHovered : handleBorderColor),
																	   handlePosition,
																	   0,
																	   outerHandleWidth,
																	   outerHandleHeight,
																	   1,
																	   handleCornerRoundness);

			// Ensures the layer bounds don't change when the handle is near either end of the track.
			// Also extends hit detection for narrow tracks, and to cover the gap between the slider and its label.
			VuoLayer spacerLayer = VuoLayer_makeColor(VuoText_make("Spacer"),
													  (VuoColor){ 0, 0, 0, 0 },
													  (VuoPoint2d){ 0, (float)-marginBetweenTrackAndLabel / 2 },
													  0,
													  outerHandleWidth  + (orientation == VuoOrientation_Horizontal ? hml : 0),
													  outerHandleHeight + (orientation == VuoOrientation_Horizontal ? 0 : hml) + marginBetweenTrackAndLabel);

			VuoListAppendValue_VuoLayer(layers, spacerLayer);
			VuoListAppendValue_VuoLayer(layers, handleBorderLayer);
			VuoListAppendValue_VuoLayer(layers, handleLayer);
		}


		if (!VuoText_isEmpty(label))
		{
			VuoFont f = labelFont;
			if (isHovered)
				f.color = (VuoColor){ f.color.r * labelColorHovered.r,
									  f.color.g * labelColorHovered.g,
									  f.color.b * labelColorHovered.b,
									  f.color.a * labelColorHovered.a };
			else
				f.color = (VuoColor){ f.color.r * labelColor.r,
									  f.color.g * labelColor.g,
									  f.color.b * labelColor.b,
									  f.color.a * labelColor.a };

			float yOffset = -marginBetweenTrackAndLabel;
			if (orientation == VuoOrientation_Horizontal)
				yOffset -= fmax(trackDepth + trackBorderThickness, handleHeight + handleBorderThickness) / 2;
			else
				yOffset -= fmax(trackLength + trackBorderThickness, handleMovementLength(trackLength) + handleWidth + handleBorderThickness) / 2;

			VuoSceneObject textLayer = VuoSceneText_make(label, f, true, INFINITY, VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Top));
			VuoSceneObject_setTransform(textLayer, VuoTransform_makeEuler((VuoPoint3d){ 0, yOffset, 0 },
																					  (VuoPoint3d){ 0, 0, 0 },
																					  (VuoPoint3d){ 1, 1, 1 }));
			VuoListAppendValue_VuoLayer(layers, (VuoLayer)textLayer);
		}

		VuoLayer groupLayer = VuoLayer_makeGroup(layers, VuoTransform2d_make(position, 0, VuoPoint2d_make(1, 1)));

		unsigned long int pixelsWide, pixelsHigh;
		float backingScaleFactor;
		if (!VuoRenderedLayers_getRenderingDimensions(renderedLayers, &pixelsWide, &pixelsHigh, &backingScaleFactor))
		{
			VuoRetain(groupLayer);
			VuoRelease(groupLayer);
			return nullptr;
		}

		return VuoLayer_setAnchor(groupLayer, anchor, pixelsWide, pixelsHigh, backingScaleFactor);
	}

	/**
	 * Returns true if `pointToTest` is inside the drag handle.
	 */
	bool isPointInsideSliderHandle(VuoRenderedLayers renderedLayers,
								   VuoReal trackLength,
								   VuoReal normalizedProgress,
								   VuoPoint2d position,
								   VuoAnchor anchor,
								   VuoOrientation orientation,
								   VuoPoint2d pointToTest)
	{
		float width  = orientation == VuoOrientation_Horizontal ? handleWidth : handleHeight;
		float height = orientation == VuoOrientation_Horizontal ? handleHeight : handleWidth;

		float hml = handleMovementLength(trackLength);
		float handlePos           = VuoReal_lerp(-hml / 2, hml / 2, normalizedProgress);
		VuoPoint2d handlePosition = (VuoPoint2d){
			orientation == VuoOrientation_Horizontal ? handlePos : 0,
			orientation == VuoOrientation_Horizontal ? 0 : handlePos,
		};

		float outerHandleWidth  = width + handleBorderThickness * 2;
		float outerHandleHeight = height + handleBorderThickness * 2;

		return VuoRectangle_isPointInside((VuoRectangle){ handlePosition, { outerHandleWidth, outerHandleHeight } },
										  (VuoPoint2d){ pointToTest.x - position.x, pointToTest.y - position.y });
	}

	/**
	 * Returns the minimum length, in local Vuo Coordinates, that the track can be.
	 */
	VuoReal minimumTrackLength()
	{
		return trackDepth * trackCornerRoundness;
	}

	/**
	 * Returns the length, in local Vuo Coordinates, that the drag handle can move.
	 * For the Rounded theme, the drag handle can move as far as the center point of the track's rounded corner arcs.
	 */
	VuoReal handleMovementLength(VuoReal trackLength)
	{
		return trackLength - trackDepth * trackCornerRoundness;
	}
};

VuoSerializableRegister(VuoUiThemeSliderRounded); ///< Register with base class

/**
 * Creates a theme for button widgets, with the rounded style.
 */
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

										VuoReal marginBetweenTrackAndLabel)
{
	return reinterpret_cast<VuoUiTheme>(new VuoUiThemeSliderRounded(labelFont,
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

																	marginBetweenTrackAndLabel));
}
