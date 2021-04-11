/**
 * @file
 * Private VuoUiThemeTextFieldRounded implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
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
	VuoFont font;
	VuoAnchor textAnchor;
	VuoPoint2d textPadding;
	VuoColor textColor;
	VuoColor textColorHovered;
	VuoColor textColorActive;
	VuoColor backgroundColor;
	VuoColor backgroundColorHovered;
	VuoColor backgroundColorActive;
	VuoColor borderColor;
	VuoColor borderColorHovered;
	VuoColor borderColorActive;
	VuoReal borderThickness;
	VuoColor cursorColor;
	VuoColor selectionColor;
	VuoReal cornerRoundness;

public:
	static std::string type; ///< The subtype's class name.

	/**
	 * Creates a rounded text field theme from JSON.
	 */
	static VuoSerializable *makeFromJson(json_object *js)
	{
		return new VuoUiThemeTextFieldRounded(
			VuoJson_getObjectValue(VuoFont,    js, "font",                   (VuoFont){VuoText_make("Avenir-Medium"), 24, false, (VuoColor){1,1,1,1}, VuoHorizontalAlignment_Left, 1, 1}),
			VuoJson_getObjectValue(VuoAnchor,  js, "textAnchor",             VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Top)),
			VuoJson_getObjectValue(VuoPoint2d, js, "textPadding",            (VuoPoint2d){0.02, 0.01}),
			VuoJson_getObjectValue(VuoColor,   js, "textColor",              (VuoColor){0, 0, 0, 0.7}),
			VuoJson_getObjectValue(VuoColor,   js, "textColorHovered",       (VuoColor){0, 0, 0, 0.8}),
			VuoJson_getObjectValue(VuoColor,   js, "textColorActive",        (VuoColor){0, 0, 0, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "backgroundColor",        (VuoColor){1, 1, 1, 0.5}),
			VuoJson_getObjectValue(VuoColor,   js, "backgroundColorHovered", (VuoColor){1, 1, 1, 0.6}),
			VuoJson_getObjectValue(VuoColor,   js, "backgroundColorActive",  (VuoColor){1, 1, 1, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "borderColor",            (VuoColor){.46, .46, .46, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "borderColorHovered",     (VuoColor){.46, .48, .49, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "borderColorActive",      (VuoColor){.46, .48, 1,   1}),
			VuoJson_getObjectValue(VuoReal,    js, "borderThickness",        .005),
			VuoJson_getObjectValue(VuoColor,   js, "cursorColor",            (VuoColor){0, 0, 0, 1}),
			VuoJson_getObjectValue(VuoColor,   js, "selectionColor",         (VuoColor){.7,  .84, 1,   1}),
			VuoJson_getObjectValue(VuoReal,    js, "cornerRoundness",        .5));
	}

	/**
	 * Creates a theme for text field widgets, with the rounded style.
	 */
	VuoUiThemeTextFieldRounded(VuoFont _font,
							   VuoAnchor _textAnchor,
							   VuoPoint2d _textPadding,
							   VuoColor _textColor,
							   VuoColor _textColorHovered,
							   VuoColor _textColorActive,
							   VuoColor _backgroundColor,
							   VuoColor _backgroundColorHovered,
							   VuoColor _backgroundColorActive,
							   VuoColor _borderColor,
							   VuoColor _borderColorHovered,
							   VuoColor _borderColorActive,
							   VuoReal _borderThickness,
							   VuoColor _cursorColor,
							   VuoColor _selectionColor,
							   VuoReal _cornerRoundness)
	{
		font = _font;
		VuoFont_retain(font);
		textAnchor = _textAnchor;
		textPadding = _textPadding;
		textColor = _textColor;
		textColorHovered = _textColorHovered;
		textColorActive = _textColorActive;
		backgroundColor = _backgroundColor;
		backgroundColorHovered = _backgroundColorHovered;
		backgroundColorActive = _backgroundColorActive;
		borderColor = _borderColor;
		borderColorHovered = _borderColorHovered;
		borderColorActive = _borderColorActive;
		borderThickness = _borderThickness;
		cursorColor = _cursorColor;
		selectionColor = _selectionColor;
		cornerRoundness = _cornerRoundness;
	}

	~VuoUiThemeTextFieldRounded()
	{
		VuoFont_release(font);
	}

	/**
	 * Encodes the theme as a JSON object.
	 */
	json_object *getJson()
	{
		json_object *json = VuoSerializable::getJson();
		json_object_object_add(json, "font", VuoFont_getJson(font));
		json_object_object_add(json, "textAnchor", VuoAnchor_getJson(textAnchor));
		json_object_object_add(json, "textPadding", VuoPoint2d_getJson(textPadding));
		json_object_object_add(json, "textColor", VuoColor_getJson(textColor));
		json_object_object_add(json, "textColorHovered", VuoColor_getJson(textColorHovered));
		json_object_object_add(json, "textColorActive", VuoColor_getJson(textColorActive));
		json_object_object_add(json, "backgroundColor", VuoColor_getJson(backgroundColor));
		json_object_object_add(json, "backgroundColorHovered", VuoColor_getJson(backgroundColorHovered));
		json_object_object_add(json, "backgroundColorActive", VuoColor_getJson(backgroundColorActive));
		json_object_object_add(json, "borderColor", VuoColor_getJson(borderColor));
		json_object_object_add(json, "borderColorHovered", VuoColor_getJson(borderColorHovered));
		json_object_object_add(json, "borderColorActive", VuoColor_getJson(borderColorActive));
		json_object_object_add(json, "borderThickness", VuoReal_getJson(borderThickness));
		json_object_object_add(json, "cursorColor", VuoColor_getJson(cursorColor));
		json_object_object_add(json, "selectionColor", VuoColor_getJson(selectionColor));
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
		return VuoFont_areEqual(font, thatSpecialized->font)
			&& VuoAnchor_areEqual(textAnchor, thatSpecialized->textAnchor)
			&& VuoPoint2d_areEqual(textPadding, thatSpecialized->textPadding)
			&& VuoColor_areEqual(textColor, thatSpecialized->textColor)
			&& VuoColor_areEqual(textColorHovered, thatSpecialized->textColorHovered)
			&& VuoColor_areEqual(textColorActive, thatSpecialized->textColorActive)
			&& VuoColor_areEqual(backgroundColor, thatSpecialized->backgroundColor)
			&& VuoColor_areEqual(backgroundColorHovered, thatSpecialized->backgroundColorHovered)
			&& VuoColor_areEqual(backgroundColorActive, thatSpecialized->backgroundColorActive)
			&& VuoColor_areEqual(borderColor, thatSpecialized->borderColor)
			&& VuoColor_areEqual(borderColorHovered, thatSpecialized->borderColorHovered)
			&& VuoColor_areEqual(borderColorActive, thatSpecialized->borderColorActive)
			&& VuoReal_areEqual(borderThickness, thatSpecialized->borderThickness)
			&& VuoColor_areEqual(cursorColor, thatSpecialized->cursorColor)
			&& VuoColor_areEqual(selectionColor, thatSpecialized->selectionColor)
			&& VuoReal_areEqual(cornerRoundness, thatSpecialized->cornerRoundness);
	}

	/**
	 * Returns true if this theme sorts before `that` theme.
	 */
	bool operator<(const VuoSerializable &that)
	{
		VuoSerializableLessThan(VuoUiThemeTextFieldRounded);
		VuoType_returnInequality(VuoFont,    font,                   thatSpecialized->font);
		VuoType_returnInequality(VuoAnchor,  textAnchor,             thatSpecialized->textAnchor);
		VuoType_returnInequality(VuoPoint2d, textPadding,            thatSpecialized->textPadding);
		VuoType_returnInequality(VuoColor,   textColor,              thatSpecialized->textColor);
		VuoType_returnInequality(VuoColor,   textColorHovered,       thatSpecialized->textColorHovered);
		VuoType_returnInequality(VuoColor,   textColorActive,        thatSpecialized->textColorActive);
		VuoType_returnInequality(VuoColor,   backgroundColor,        thatSpecialized->backgroundColor);
		VuoType_returnInequality(VuoColor,   backgroundColorHovered, thatSpecialized->backgroundColorHovered);
		VuoType_returnInequality(VuoColor,   backgroundColorActive,  thatSpecialized->backgroundColorActive);
		VuoType_returnInequality(VuoColor,   borderColor,            thatSpecialized->borderColor);
		VuoType_returnInequality(VuoColor,   borderColorHovered,     thatSpecialized->borderColorHovered);
		VuoType_returnInequality(VuoColor,   borderColorActive,      thatSpecialized->borderColorActive);
		VuoType_returnInequality(VuoReal,    borderThickness,        thatSpecialized->borderThickness);
		VuoType_returnInequality(VuoColor,   cursorColor,            thatSpecialized->cursorColor);
		VuoType_returnInequality(VuoColor,   selectionColor,         thatSpecialized->selectionColor);
		VuoType_returnInequality(VuoReal,    cornerRoundness,        thatSpecialized->cornerRoundness);
		return false;
	}

	/**
	 * Creates a layer tree representing a text field with the specified theme and parameters.
	 *	@c cursorIndex, @c selectionStart, and @c selectionEnd are all 0 indexed.
	 */
	VuoLayer render(VuoPoint2d screenSize,
					VuoReal screenBackingScaleFactor,
					VuoText text,
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

		bool hasText = VuoText_length(text) > 0;
		bool hasPlaceholder = VuoText_length(placeholderText) > 0;
		VuoText labelText;
		if (hasText)
			labelText = text;
		else if(isFocused || !hasPlaceholder)
			labelText = VuoText_make("");
		else
			labelText = placeholderText;
		VuoLocal(labelText);

		float actualWidth = fmax(width, 0.001);
		float actualHeight;

		VuoPoint2d textSize = VuoPoint2d_make(0,0);

		VuoFont f = font;
		if (isFocused)
			f.color = (VuoColor){f.color.r * textColorActive.r,
								 f.color.g * textColorActive.g,
								 f.color.b * textColorActive.b,
								 f.color.a * textColorActive.a};
		else if (isHovered)
			f.color = (VuoColor){f.color.r * textColorHovered.r,
								 f.color.g * textColorHovered.g,
								 f.color.b * textColorHovered.b,
								 f.color.a * textColorHovered.a};
		else
			f.color = (VuoColor){f.color.r * textColor.r,
								 f.color.g * textColor.g,
								 f.color.b * textColor.b,
								 f.color.a * textColor.a};

		if (!hasText)
			f.color.a *= .25;

		VuoImageTextData textData = VuoImage_getTextImageData(labelText, f, screenBackingScaleFactor, 1, 0, true);

		if(textData)
		{
			textData->billboardAnchor = textAnchor;
			VuoImageTextData_convertToVuoCoordinates(textData, screenSize.x, screenBackingScaleFactor);

			textSize.x = textData->width;
			textSize.y = textData->height;

			// Expand the text field width, if needed, to accommodate long text.
			// (Scrolling would be preferable, but would take much longer to implement.)
			actualWidth = fmax(width, textData->width);

			actualHeight = textData->lineHeight * numLines;

			VuoSceneObject text = VuoSceneText_make(labelText, f, true, INFINITY, textAnchor);
			textLayer = (VuoLayer)text;
			VuoSceneObject_setName((VuoSceneObject)textLayer, VuoText_make("Text"));
		}
		else
		{
			VuoReal lineHeight = VuoImageText_getLineHeight(f, screenSize.x, screenBackingScaleFactor);
			actualHeight = lineHeight * numLines;
			textSize.y = lineHeight;
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
			VuoPoint2d cursorSize = VuoImageText_getTextSize("|", f, screenSize, screenBackingScaleFactor, false);
			cursorSize.x *= .5f;

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

			int textLength = VuoText_length(text);
			selectionStart = MIN(selectionStart, textLength);
			selectionEnd   = MIN(selectionEnd,   textLength);

			int start = (selectionStart == selectionEnd ? cursorIndex : MIN(selectionStart, selectionEnd));
			int length = (selectionStart == selectionEnd ? 0 : (MAX(selectionStart, selectionEnd) - start));

			if (hasText && length > 0)
			{
				unsigned int lineCount = 0;
				VuoRectangle* highlightRects = VuoImageTextData_getRectsForHighlight(textData, start, length, &lineCount);

				for(int i = 0; i < lineCount; i++)
				{
					VuoListAppendValue_VuoLayer(
						highlights,
						VuoLayer_makeColor(VuoText_make(VuoText_format("Text Highlight %i", i)),
							selectionColor,
							highlightRects[i].center,
							0,
							highlightRects[i].size.x,
							highlightRects[i].size.y) );
				}

				free(highlightRects);
			}
		}

		*imageTextData = textData;

		actualWidth  += textPadding.x * 2;
		actualHeight += textPadding.y * 2;

		actualWidth  += borderThickness * 2;
		actualHeight += borderThickness * 2;

		float outerCornerRoundness = cornerRoundness / numLines;
		float innerCornerRoundness = (actualHeight - borderThickness * 2 - (actualHeight * (1 - outerCornerRoundness))) / (actualHeight - borderThickness * 2);

		VuoLayer backgroundLayer = VuoLayer_makeRoundedRectangle(   VuoText_make("Text Field Background"),
																	isFocused ? backgroundColorActive : (isHovered ? backgroundColorHovered : backgroundColor),
																	VuoPoint2d_make(0,0),
																	0,
																	actualWidth  - borderThickness * 2,
																	actualHeight - borderThickness * 2,
																	1,
																	innerCornerRoundness);

		VuoLayer borderLayer = VuoLayer_makeRoundedRectangle(   VuoText_make("Text Field Border"),
																isFocused ? borderColorActive : (isHovered ? borderColorHovered : borderColor),
																VuoPoint2d_make(0,0),
																0,
																actualWidth,
																actualHeight,
																1,
																outerCornerRoundness);

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

		{
			VuoSceneObject_translate((VuoSceneObject)textLayer, offset);
			for(auto& highlight : *((std::vector<VuoLayer> *) highlights))
				VuoSceneObject_translate((VuoSceneObject)highlight, offset);
			VuoSceneObject_translate((VuoSceneObject)textCursor, offset);

			VuoHorizontalAlignment h = VuoAnchor_getHorizontal(textAnchor);
			VuoVerticalAlignment v = VuoAnchor_getVertical(textAnchor);

			if(h != VuoHorizontalAlignment_Center)
				textOffset.x = ((actualWidth - textPadding.x * 2) * .5) * (h == VuoHorizontalAlignment_Left ? -1 : 1);

			if(v != VuoVerticalAlignment_Center)
				textOffset.y = ((actualHeight - textPadding.y * 2) * .5) * (v == VuoVerticalAlignment_Bottom ? -1 : 1);

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

		if (hasText || hasPlaceholder)
		{
			for(unsigned int i = 0; i < VuoListGetCount_VuoLayer(highlights); i++)
				VuoListAppendValue_VuoLayer(layers, VuoListGetValue_VuoLayer(highlights, i+1));
			VuoListAppendValue_VuoLayer(layers, textLayer);
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
										   VuoReal cornerRoundness)
{
	return reinterpret_cast<VuoUiTheme>(new VuoUiThemeTextFieldRounded(font,
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
																	   cornerRoundness));
}
