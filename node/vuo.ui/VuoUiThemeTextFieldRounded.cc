/**
 * @file
 * Private VuoUiThemeTextFieldRounded implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

/// Draws per-character bounding boxes.
#define THEME_DEBUG 0

#include "VuoUiThemeBase.hh"

#include "type.h"

extern "C" {
#include "VuoImage.h"
#include "VuoImageText.h"
#include "VuoAnchor.h"
#include "VuoPoint2d.h"
}

#include "VuoUiTheme.h"
#include "VuoSceneText.h"
#include <vector>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title": "UI Theme: Text Field, Rounded",
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
 * A theme for text/number widgets, with the rounded style.
 */
class VuoUiThemeTextFieldRounded : public VuoUiThemeTextField
{
private:
	VuoFont labelFont;
	VuoFont placeholderFont;
	VuoAnchor labelPosition;
	VuoPoint2d labelPadding;
	VuoColor color;
	VuoColor hoveredColor;
	VuoColor pressedColor;
	VuoColor borderColor;
	VuoReal borderThickness;
	VuoReal cornerRoundness;

public:
	static std::string type; ///< The subtype's class name.

	/**
	 * Creates a rounded button theme from JSON.
	 */
	static VuoSerializable *makeFromJson(json_object *js)
	{
		return new VuoUiThemeTextFieldRounded(
			VuoJson_getObjectValue(VuoFont,    js, "labelFont",       (VuoFont){NULL, 18, false, VuoColor_makeWithRGBA(0,0,0,1), VuoHorizontalAlignment_Left, 1, 1}),
			VuoJson_getObjectValue(VuoFont,    js, "placeholderFont", (VuoFont){NULL, 18, false, VuoColor_makeWithRGBA(0, 0, 0, 0.25), VuoHorizontalAlignment_Left, 1, 1}),
			VuoJson_getObjectValue(VuoAnchor,  js, "labelPosition",   VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Center)),
			VuoJson_getObjectValue(VuoPoint2d, js, "labelPadding",    (VuoPoint2d){0.05, 0.05}),
			VuoJson_getObjectValue(VuoColor,   js, "color",           (VuoColor){.95, .95, .95, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "hoveredColor",    (VuoColor){.41,.51,.61,1}),
			VuoJson_getObjectValue(VuoColor,   js, "pressedColor",    (VuoColor){.35,.45,.55,1}),
			VuoJson_getObjectValue(VuoColor,   js, "borderColor",     (VuoColor){.3,.4,.5,1}),
			VuoJson_getObjectValue(VuoReal,    js, "borderThickness", .2),
			VuoJson_getObjectValue(VuoReal,    js, "cornerRoundness", .2));
	}

	/**
	 * Creates a theme for button widgets, with the rounded style.
	 */
	VuoUiThemeTextFieldRounded(
		VuoFont _labelFont,
		VuoFont _placeholderFont,
		VuoAnchor _labelPosition,
		VuoPoint2d _labelPadding,
		VuoColor _color,
		VuoColor _hoveredColor,
		VuoColor _pressedColor,
		VuoColor _borderColor,
		VuoReal _borderThickness,
		VuoReal _cornerRoundness)
	{
		labelFont = _labelFont;
		VuoFont_retain(labelFont);
		placeholderFont = _placeholderFont;
		VuoFont_retain(placeholderFont);
		labelPosition = _labelPosition;
		labelPadding = _labelPadding;
		color = _color;
		hoveredColor = _hoveredColor;
		pressedColor = _pressedColor;
		borderColor = _borderColor;
		borderThickness = _borderThickness;
		cornerRoundness = _cornerRoundness;
	}

	~VuoUiThemeTextFieldRounded()
	{
		VuoFont_release(labelFont);
		VuoFont_release(placeholderFont);
	}

	/**
	 * Encodes the theme as a JSON object.
	 */
	json_object *getJson()
	{
		json_object *json = VuoSerializable::getJson();
		json_object_object_add(json, "labelFont", VuoFont_getJson(labelFont));
		json_object_object_add(json, "placeholderFont", VuoFont_getJson(placeholderFont));
		json_object_object_add(json, "labelPosition", VuoAnchor_getJson(labelPosition));
		json_object_object_add(json, "labelPadding", VuoPoint2d_getJson(labelPadding));
		json_object_object_add(json, "color", VuoColor_getJson(color));
		json_object_object_add(json, "hoveredColor", VuoColor_getJson(hoveredColor));
		json_object_object_add(json, "pressedColor", VuoColor_getJson(pressedColor));
		json_object_object_add(json, "borderColor", VuoColor_getJson(borderColor));
		json_object_object_add(json, "borderThickness", VuoReal_getJson(borderThickness));
		json_object_object_add(json, "cornerRoundness", VuoReal_getJson(cornerRoundness));
		return json;
	}

	/**
	 * Returns a compact string representation of the theme.
	 */
	char *getSummary()
	{
		return strdup("Text Field Theme (Rounded)");
	}

	/**
	 * Returns true if both themes are of the same subtype, and their values are equal.
	 */
	bool operator==(const VuoSerializable &that)
	{
		VuoSerializableEquals(VuoUiThemeTextFieldRounded);
		return VuoFont_areEqual(labelFont, thatSpecialized->labelFont)
			&& VuoFont_areEqual(placeholderFont, thatSpecialized->placeholderFont)
			&& VuoAnchor_areEqual(labelPosition, thatSpecialized->labelPosition)
			&& VuoPoint2d_areEqual(labelPadding, thatSpecialized->labelPadding)
			&& VuoColor_areEqual(color, thatSpecialized->color)
			&& VuoColor_areEqual(hoveredColor, thatSpecialized->hoveredColor)
			&& VuoColor_areEqual(pressedColor, thatSpecialized->pressedColor)
			&& VuoColor_areEqual(borderColor, thatSpecialized->borderColor)
			&& VuoReal_areEqual(borderThickness, thatSpecialized->borderThickness)
			&& VuoReal_areEqual(cornerRoundness, thatSpecialized->cornerRoundness);
	}

	/**
	 * Returns true if this theme sorts before `that` theme.
	 */
	bool operator<(const VuoSerializable &that)
	{
		VuoSerializableLessThan(VuoUiThemeTextFieldRounded);
		VuoType_returnInequality(VuoFont,    labelFont,       thatSpecialized->labelFont);
		VuoType_returnInequality(VuoFont,    placeholderFont, thatSpecialized->placeholderFont);
		VuoType_returnInequality(VuoAnchor,  labelPosition,   thatSpecialized->labelPosition);
		VuoType_returnInequality(VuoPoint2d, labelPadding,    thatSpecialized->labelPadding);
		VuoType_returnInequality(VuoColor,   color,           thatSpecialized->color);
		VuoType_returnInequality(VuoColor,   hoveredColor,    thatSpecialized->hoveredColor);
		VuoType_returnInequality(VuoColor,   pressedColor,    thatSpecialized->pressedColor);
		VuoType_returnInequality(VuoColor,   borderColor,     thatSpecialized->borderColor);
		VuoType_returnInequality(VuoReal,    borderThickness, thatSpecialized->borderThickness);
		VuoType_returnInequality(VuoReal,    cornerRoundness, thatSpecialized->cornerRoundness);
		return false;
	}

	/**
	 * Creates a layer tree representing a button with the specified theme and parameters.
	 *	@c cursorIndex, @c selectionStart, and @c selectionEnd are all 0 indexed.
	 */
	VuoLayer render(VuoPoint2d screenSize,
					VuoReal screenBackingScaleFactor,
					VuoColor cursorColor,
					VuoText label,
					VuoText placeholderText,
					int numLines,
					int cursorIndex,
					int selectionStart,
					int selectionEnd,
					VuoPoint2d position,
					VuoReal width,
					VuoAnchor anchor,
					bool isHovered,
					bool isFocused,
					VuoImageTextData* imageTextData)
	{
		VuoList_VuoLayer layers = VuoListCreate_VuoLayer();
		VuoLocal(layers);

		VuoLayer textLayer = nullptr;
		VuoList_VuoLayer highlights = VuoListCreate_VuoLayer();
		VuoRetain(highlights);
		VuoLayer textCursor = nullptr;

		bool hasLabel = VuoText_length(label) > 0;
		bool hasPlaceholder = VuoText_length(placeholderText) > 0;
		VuoText labelText;
		if(hasLabel)
			labelText = label;
		else if(isFocused || !hasPlaceholder)
			labelText = VuoText_make("");
		else
			labelText = placeholderText;
		VuoLocal(labelText);

		float actualWidth = fmax(width, 0.001);
		float actualHeight;

		VuoPoint2d textSize = VuoPoint2d_make(0,0);

		VuoFont& font = ((hasLabel || isFocused) ? labelFont : placeholderFont);
		VuoImageTextData textData = VuoImage_getTextImageData(labelText, font, screenBackingScaleFactor, 1, 0, false);

		if(textData)
		{
			textData->billboardAnchor = labelPosition;
			VuoImageTextData_convertToVuoCoordinates(textData, screenSize.x, screenBackingScaleFactor);

			textSize.x = textData->width;
			textSize.y = textData->height;

			// actualWidth = fmax(width, textData->width);
			actualHeight = textData->lineHeight * numLines;

			VuoSceneObject text = VuoSceneText_make(labelText, font, true, INFINITY, labelPosition);
			textLayer = (VuoLayer)text;
			VuoSceneObject_setName((VuoSceneObject)textLayer, VuoText_make("Text"));
		}
		else
		{
			VuoReal lineHeight = VuoImageText_getLineHeight(font, screenSize.x, screenBackingScaleFactor);
			actualHeight = lineHeight * numLines;
		}

#if THEME_DEBUG
		VuoList_VuoLayer characterBounds = VuoListCreate_VuoLayer();
		VuoLocal(characterBounds);

		if(textData)
		{
			for(int i = 0; i < textData->charCount; i++)
			{
				// don't draw control characters
				if(textData->charAdvance[i] < .001)
					continue;

				// getPositionForCharIndex returns the bottom left corner of the char billboard
				unsigned int lineIndex;
				VuoPoint2d p = VuoImageTextData_getPositionForCharIndex(textData, i, &lineIndex);
				p.x += textData->charAdvance[i] * .5;
				p.y += textData->lineHeight * .5;

				VuoListAppendValue_VuoLayer(characterBounds,
											VuoLayer_makeColor(VuoText_make(VuoText_format("line %i", i)),
											VuoColor_makeWithRGBA(.2,.2,.2,.4),
											p,
											0,
											textData->charAdvance[i] * .95,
											textData->lineHeight * .95));
			}
		}
#endif

		// if the text isn't placeholder, also render the cursor and selection (if any)
		if(isFocused)
		{
			VuoPoint2d cursorSize = VuoImageText_getTextSize("|", font, screenSize, screenBackingScaleFactor, false);
			cursorSize.x *= .15f;

			VuoPoint2d cursorPosition;

			if(textData)
			{
				cursorPosition = VuoImageTextData_getPositionForCharIndex(textData, cursorIndex, NULL);
				cursorPosition.y += textData->lineHeight * .5;
			}
			else
			{
				cursorPosition = VuoPoint2d_make(0, 0);
			}

			textCursor = VuoLayer_makeColor(VuoText_make("Text Cursor"),
				cursorColor,
				cursorPosition,
				0,
				cursorSize.x,
				cursorSize.y);

			int start = (selectionStart == selectionEnd ? cursorIndex : MIN(selectionStart, selectionEnd));
			int length = (selectionStart == selectionEnd ? 0 : (MAX(selectionStart, selectionEnd) - start));

			if(hasLabel && length > 0)
			{
				unsigned int lineCount = 0;
				VuoRectangle* highlightRects = VuoImageTextData_getRectsForHighlight(textData, start, length, &lineCount);

				for(int i = 0; i < lineCount; i++)
				{
					VuoListAppendValue_VuoLayer(
						highlights,
						VuoLayer_makeColor(VuoText_make(VuoText_format("Text Highlight %i", i)),
							VuoColor_makeWithRGBA(borderColor.r, borderColor.g, borderColor.b, .6),
							highlightRects[i].center,
							0,
							highlightRects[i].size.x,
							highlightRects[i].size.y) );
				}

				free(highlightRects);
			}
		}

		*imageTextData = textData;

		actualWidth += labelPadding.x;
		actualHeight += labelPadding.y;

		float border = borderThickness * fmin(actualWidth, actualHeight);

		VuoLayer backgroundLayer = VuoLayer_makeRoundedRectangle(	VuoText_make("Button Background"),
																	color,
																	VuoPoint2d_make(0,0),
																	0,
																	actualWidth - border,
																	actualHeight - border,
																	1,
																	cornerRoundness);

		VuoLayer borderLayer = VuoLayer_makeRoundedRectangle(	VuoText_make("Button Border"),
																borderColor,
																VuoPoint2d_make(0,0),
																0,
																actualWidth,
																actualHeight,
																1,
																cornerRoundness);

		VuoPoint3d offset = VuoPoint3d_make(0,0,0);
		VuoPoint3d textOffset = VuoPoint3d_make(0,0,0);

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

		if(hasLabel || hasPlaceholder)
		{
			VuoSceneObject_translate((VuoSceneObject)textLayer, offset);
			for(auto& highlight : *((std::vector<VuoLayer> *) highlights))
				VuoSceneObject_translate((VuoSceneObject)highlight, offset);
			VuoSceneObject_translate((VuoSceneObject)textCursor, offset);

			VuoHorizontalAlignment h = VuoAnchor_getHorizontal(labelPosition);
			VuoVerticalAlignment v = VuoAnchor_getVertical(labelPosition);

			if(h != VuoHorizontalAlignment_Center)
				textOffset.x = ((actualWidth - labelPadding.x) * .5) * (h == VuoHorizontalAlignment_Left ? -1 : 1);

			if(v != VuoVerticalAlignment_Center)
				textOffset.y = ((actualHeight - labelPadding.y) * .5) * (v == VuoVerticalAlignment_Bottom ? -1 : 1);

			VuoSceneObject_translate((VuoSceneObject)textLayer, textOffset);

			// the text layer is already transformed by the text renderer to account for font anchor,
			// so add it after the alignment of text billboard for highlight and cursor
			if(h != VuoHorizontalAlignment_Center)
				textOffset.x += (h == VuoHorizontalAlignment_Left ? textSize.x : -textSize.x) * .5;

			if(v != VuoVerticalAlignment_Center)
				textOffset.y += (v == VuoVerticalAlignment_Bottom ? textSize.y : -textSize.y) * .5;

			for(auto& highlight : *((std::vector<VuoLayer> *) highlights))
				VuoSceneObject_translate((VuoSceneObject)highlight, textOffset);

			VuoSceneObject_translate((VuoSceneObject)textCursor, textOffset);
		}

		VuoListAppendValue_VuoLayer(layers, borderLayer);
		VuoListAppendValue_VuoLayer(layers, backgroundLayer);

#if THEME_DEBUG
		for(int i = 1; i <= VuoListGetCount_VuoLayer(characterBounds); i++)
		{
			VuoLayer l = VuoListGetValue_VuoLayer(characterBounds, i);
			l.sceneObject.transform.translation = VuoPoint3d_add(l.sceneObject.transform.translation, offset);

			// VLog("textOffset [%i]: %.2f  %.2f", i, textOffset.x, textOffset.y);
			l.sceneObject.transform.translation.x += textOffset.x;
			l.sceneObject.transform.translation.y += textOffset.y;

			VuoListAppendValue_VuoLayer(layers, l);
		}
#endif

		if(hasLabel || hasPlaceholder)
		{
			VuoListAppendValue_VuoLayer(layers, textLayer);
			for(unsigned int i = 0; i < VuoListGetCount_VuoLayer(highlights); i++)
				VuoListAppendValue_VuoLayer(layers, VuoListGetValue_VuoLayer(highlights, i+1));
			VuoListAppendValue_VuoLayer(layers, textCursor);
		}

		VuoRelease(highlights);

		return VuoLayer_makeGroup(layers, VuoTransform2d_make(position, 0, VuoPoint2d_make(1,1)));
	}
};

VuoSerializableRegister(VuoUiThemeTextFieldRounded);	///< Register with base class

/**
 * Creates a theme for text/number field widgets, with the rounded style.
 */
VuoUiTheme VuoUiTheme_makeTextFieldRounded(
	VuoFont labelFont,
	VuoFont placeholderFont,
	VuoAnchor labelAnchor,
	VuoPoint2d labelPadding,
	VuoColor color,
	VuoColor hoveredColor,
	VuoColor pressedColor,
	VuoColor borderColor,
	VuoReal borderThickness,
	VuoReal cornerRoundness)
{
	return reinterpret_cast<VuoUiTheme>(new VuoUiThemeTextFieldRounded(
		labelFont,
		placeholderFont,
		labelAnchor,
		labelPadding,
		color,
		hoveredColor,
		pressedColor,
		borderColor,
		borderThickness,
		cornerRoundness));
}
