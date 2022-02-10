/**
 * @file
 * VuoTextField internal implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "node.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoText.h"
#include "VuoLayer.h"
#include "VuoImageText.h"
#include "VuoColor.h"
#include "VuoUiTheme.h"
#include "VuoRenderedLayers.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include "VuoTextEdit.h"
#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb_textedit.h"
#pragma clang diagnostic pop

/**
 *	Hold information about the current UI state of a text field (hovering, focused, etc).
 */
struct VuoTextFieldState
{
public:
	VuoText text;						///< The text to be displayed in the field.
	VuoText placeholder;				///< When no user set text is available, display this text as a placeholder.
	VuoImageTextData textImageData;		///< Store information about rendered text in image form.
	STB_TexteditState textEditState;	///< stb_textedit state.
	bool isHovering;					///< Is this field currently hovered?
	bool isFocused;						///< Is this field currently focused?

	/// Text field state constructor.
	VuoTextFieldState(int lines);

	/// Text field state destructor.
	~VuoTextFieldState();
};

/**
 *	Internal implementation of a text field object.
 */
class VuoTextFieldInternal
{
private:
	void *context;                    ///< Caller-provided context.
	VuoTextFieldState* state;         ///< The current state of this text field.
	uint64_t id;
	VuoPoint2d screenSize;            ///< Last known screen size.
	VuoReal screenBackingScaleFactor; ///< Last known screen backing scale factor.
	VuoPoint2d position;              ///< Layer position.
	VuoReal width;                    ///< Layer width.
	VuoAnchor anchor;                 ///< Layer anchor.
	VuoUiTheme theme;                 ///< UI Text Field theme to use when rendering.
	VuoInteger lineCount;             ///< The number of lines this text field allows.

	/// Optional function callback to be invoked when OnTypedCharacterEvent is fired.  Returns true if input is valid, false otherwise.
	bool (*validateCharInput)(const VuoText current, uint32_t newChar, uint16_t position);

	/// Optional function callback to be invoked when text field loses focus.  Use this to validate user input and (if necessary) change it.
	/// Returns true if the text has been modified (in which case @modified should contain the new text).
	bool (*validateTextInput)(void *context, const VuoText current, VuoText* modified);

	typedef void (*SessionEndedCallbackType)(void *context, VuoText text);
	SessionEndedCallbackType sessionEndedCallback;

	bool FindTextLayer(const VuoRenderedLayers* renderedLayers, uint64_t id, VuoSceneObject* textLayer, VuoList_VuoSceneObject ancestors) VuoWarnUnusedResult;
	bool GetTextLocalPosition(const VuoRenderedLayers* renderedLayers, uint64_t id, VuoPoint2d point, VuoPoint2d* inverseTransformedPoint) VuoWarnUnusedResult;
	void OnLostFocus();

public:

	VuoTextFieldInternal(VuoInteger numLines, void *context);
	~VuoTextFieldInternal();

	void SetLineCount(VuoInteger lines);
	void SetPosition(VuoPoint2d newPosition);
	void SetWidth(VuoReal newWidth);
	void SetAnchor(VuoAnchor newAnchor);
	void SetTheme(VuoUiTheme newTheme);
	void SetText(VuoText text);
	void SetPlaceholderText(VuoText text);
	void SetValidateCharInputCallback(bool (*validateCharInputCallback)(const VuoText current, uint32_t newChar, uint16_t position));
	void SetValidateTextInputCallback(bool (*validateTextInputCallback)(void *context, const VuoText current, VuoText* modifiedText));
	void setSessionEndedCallback(SessionEndedCallbackType sessionEndedCallback);

	VuoText GetText() const;

	void OnTypedCharacterEvent(VuoText character, VuoModifierKey modifiers);
	bool OnRenderedLayersEvent(const VuoRenderedLayers* renderedLayers) VuoWarnUnusedResult;
	VuoLayer CreateTextLayer();
};

#ifdef __cplusplus
}
#endif
