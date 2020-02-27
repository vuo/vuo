/**
 * @file
 * VuoTextField internal implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTextFieldInternal.h"
#include "module.h"

extern "C"
{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoTextFieldInternal",
					"dependencies" : [ "VuoKeyboard", "VuoClipboard" ],
				});
#endif
}

#include "VuoUiThemeBase.hh"

/**
 * Make and retain a VuoText string.
 */
static VuoText CreateVuoText(const char* string)
{
	VuoText t = VuoText_make(string);
	VuoRetain(t);
	return t;
}

/**
 * Initialize a new text field state.
 */
VuoTextFieldState::VuoTextFieldState(int lines)
{
	text = NULL;
	placeholder = CreateVuoText("Placeholder Text");
	textImageData = NULL;
	isHovering = false;
	isFocused = false;
	stb_textedit_initialize_state(&textEditState, lines);
}

/**
 * Release a text field state.
 */
VuoTextFieldState::~VuoTextFieldState()
{
	if(text != NULL)
		VuoRelease(text);

	if(placeholder != NULL)
		VuoRelease(placeholder);

	VuoRelease(textImageData);
}

/**
 * Initialize a new internal text field object.
 */
VuoTextFieldInternal::VuoTextFieldInternal(VuoInteger numLines)
{
	state = new VuoTextFieldState(numLines);
	lineCount = numLines;
	id = VuoSceneObject_getNextId();
	screenSize = VuoPoint2d_make(512, 512);
	screenBackingScaleFactor = 1;
	position = VuoPoint2d_make(0,0);
	anchor = VuoAnchor_makeCentered();
	theme = NULL;
	validateCharInput = NULL;
	validateTextInput = NULL;
}

/**
 * Release resources allocated by a text field object.
 */
VuoTextFieldInternal::~VuoTextFieldInternal()
{
	delete state;
}

/**
 * Set the number of lines to be displayed when rendering this field.
 */
void VuoTextFieldInternal::SetLineCount(VuoInteger lines)
{
	state->textEditState.line_count = lines;
	lineCount = lines;
	SetText(state->text);
}

/**
 * Set the color of the text cursor.
 */
void VuoTextFieldInternal::SetCursorColor(VuoColor color)
{
	cursorColor = color;
}

/**
 * Set the position used when rendering this field to a layer.
 */
void VuoTextFieldInternal::SetPosition(VuoPoint2d newPosition)
{
	position = newPosition;
}

/**
 * Set the width used when rendering this field to a layer.
 */
void VuoTextFieldInternal::SetWidth(VuoReal newWidth)
{
	width = newWidth;
}

/**
 * Set the anchor used when rendering this field to a layer.
 */
void VuoTextFieldInternal::SetAnchor(VuoAnchor newAnchor)
{
	anchor = newAnchor;
}

/**
 * Set the text for this field.
 */
void VuoTextFieldInternal::SetText(VuoText text)
{
	if(state->text != NULL)
		VuoRelease(state->text);

	state->text = VuoTextEdit_truncateLines(text, lineCount);

	VuoRetain(state->text);
}

/**
 * Return a new VuoText string of the currently displayed value.
 */
VuoText VuoTextFieldInternal::GetText() const
{
	return VuoText_make(state->text);
}

/**
 * Set the text for this field.
 */
void VuoTextFieldInternal::SetPlaceholderText(VuoText text)
{
	if(state->placeholder != NULL)
		VuoRelease(state->placeholder);
	state->placeholder = VuoText_make(text);
	VuoRetain(state->placeholder);
}

/**
 * Set the theme used when rendering this field to a layer.
 */
void VuoTextFieldInternal::SetTheme(VuoUiTheme newTheme)
{
	theme = newTheme;
}

/**
 * Set an optional function callback to be invoked when OnTypedCharacterEvent is fired.
 * Returns true if input is valid, false otherwise.
 */
void VuoTextFieldInternal::SetValidateCharInputCallback(bool (*validateCharInputCallback)(const VuoText current, uint32_t append))
{
	validateCharInput = validateCharInputCallback;
}

/**
 * Optional function callback to be invoked when text field loses focus.  Use this to validate user input and (if necessary) change it.
 * Returns true if the text has been modified (in which case modifiedText should contain the new text).
 */
void VuoTextFieldInternal::SetValidateTextInputCallback(bool (*validateTextInputCallback)(const VuoText current, VuoText* modifiedText))
{
	validateTextInput = validateTextInputCallback;
}

/**
 * Find a text layer and it's ancestors in rendered layers.
 */
bool VuoTextFieldInternal::FindTextLayer(const VuoRenderedLayers* renderedLayers, uint64_t id, VuoSceneObject* textLayer, VuoList_VuoSceneObject ancestors)
{
	VuoSceneObject foundObject;

	if( VuoRenderedLayers_findLayerId(*renderedLayers, id, ancestors, &foundObject) )
	{
		bool foundTextLayer = false;
		VuoList_VuoSceneObject childObjects = VuoSceneObject_getChildObjects(foundObject);
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(childObjects);
		for (unsigned long i = 1; i <= childObjectCount && !foundTextLayer; ++i)
		{
			VuoSceneObject child = VuoListGetValue_VuoSceneObject(childObjects, i);

			if (VuoText_areEqual("Text", VuoSceneObject_getName(child)))
			{
				VuoListAppendValue_VuoSceneObject(ancestors, foundObject);
				*textLayer = child;
				return true;
			}
		}
	}

	return false;
}

/**
 * Transform a world coordinate point to a coordinate relative to a text field layer.
 */
bool VuoTextFieldInternal::GetTextLocalPosition(const VuoRenderedLayers* renderedLayers, uint64_t id, VuoPoint2d point, VuoPoint2d* inverseTransformedPoint)
{
	VuoSceneObject textLayer;
	VuoList_VuoSceneObject ancestors = VuoListCreate_VuoSceneObject();
	VuoLocal(ancestors);

	if( FindTextLayer(renderedLayers, id, &textLayer, ancestors) )
	{
		// transform mouse position to text layer space
		return VuoRenderedLayers_getInverseTransformedPoint(*renderedLayers, ancestors, textLayer, point, inverseTransformedPoint);
	}

	return false;
}

/**
 * Update the internal text field state with a typed character.  Use CreateTextLayer to rebuild the layer after a change has been made.
 */
void VuoTextFieldInternal::OnTypedCharacterEvent(VuoText character, VuoModifierKey modifiers)
{
	if(!state->isFocused)
		return;

	size_t utf32_len;

	uint32_t* unicode = VuoText_getUtf32Values(character, &utf32_len);

	if(utf32_len != 1)
	{
		if(unicode != NULL)
			free(unicode);

		return;
	}

	uint32_t utf32_char = unicode[0];

	free(unicode);

	// VLog("char: 0x%x  mod: 0x%x  =  0x%llx", unicode[0], modifiers, VuoTextEdit_combineKeyAndModifier(unicode[0], modifiers));

	if(	utf32_char == KEYCODE_ESC ||
		(!(modifiers & VuoModifierKey_Option) && (
				utf32_char == KEYCODE_TAB ||
				utf32_char == KEYCODE_RETURN
			)
		)
	)
	{
		state->isFocused = false;
		OnLostFocus();
	}
	else if(utf32_char == KEYCODE_CUT)
	{
		VuoTextEdit_cut(&state->text, &state->textEditState);
	}
	else if(utf32_char == KEYCODE_COPY)
	{
		VuoTextEdit_copy(&state->text, &state->textEditState);
	}
	else if(utf32_char == KEYCODE_PASTE)
	{
		VuoTextEdit_paste(&state->text, &state->textEditState);
	}
	else
	{
		// if validateCharInput callback is null, char is a control code, or validInput callback return true
		if(validateCharInput == NULL || utf32_char < 32 || validateCharInput(state->text, utf32_char))
			stb_textedit_key(&state->text, &state->textEditState, VuoTextEdit_combineKeyAndModifier(utf32_char, modifiers));
	}
}

/**
 * Update the internal state of this text field with a rendered layers event.
 * Returns true if a state change has occurred that requires a layer rebuild,
 * false otherwise. Use CreateTextLayer to rebuild the layer after a change
 * has been made.
 */
bool VuoTextFieldInternal::OnRenderedLayersEvent(const VuoRenderedLayers* renderedLayers)
{
	bool stateDidChange = false;

	if(renderedLayers == NULL)
		return stateDidChange;

	unsigned long int pixelsWide, pixelsHigh;
	float backingScaleFactor;
	if (VuoRenderedLayers_getRenderingDimensions(*renderedLayers, &pixelsWide, &pixelsHigh, &backingScaleFactor)
		&& (screenSize.x != pixelsWide
		 || screenSize.y != pixelsHigh
		 || screenBackingScaleFactor != backingScaleFactor))
		stateDidChange = true;

	screenSize.x = pixelsWide;
	screenSize.y = pixelsHigh;
	screenBackingScaleFactor = backingScaleFactor;

	VuoList_VuoInteraction interactions = VuoRenderedLayers_getInteractions(*renderedLayers);
	if (VuoListGetCount_VuoInteraction(interactions) > 0)
	{
		bool wasHovering = state->isHovering;
		bool wasFocused = state->isFocused;

		// @todo currently just registers last interaction.
		for(int i = 1; i <= VuoListGetCount_VuoInteraction(interactions); i++)
		{
			VuoInteraction it = VuoListGetValue_VuoInteraction(interactions, i);

			bool isPointInLayer = VuoRenderedLayers_isPointInLayerId(*renderedLayers, id, it.position);
			state->isHovering = isPointInLayer;
			VuoPoint2d inverseTransformedPoint;

			if(it.type == VuoInteractionType_Press || it.type == VuoInteractionType_Click)
			{
				state->isFocused = isPointInLayer;

				if( state->isFocused && GetTextLocalPosition(renderedLayers, id, it.position, &inverseTransformedPoint) )
				{
					if( it.clickCount == 1 )
						stb_textedit_click(&state->text, &state->textEditState, inverseTransformedPoint.x, inverseTransformedPoint.y);
					else if( it.clickCount == 2 )
						VuoTextEdit_selectFromCursorToSeparator(&state->text, &state->textEditState, &VuoTextEdit_isSeparator);
					else if( it.clickCount == 3 )
						VuoTextEdit_selectFromCursorToSeparator(&state->text, &state->textEditState, &VuoTextEdit_isNewLine);

					stateDidChange = true;
				}
			}
			else if(state->isFocused && it.type == VuoInteractionType_Drag)
			{
				if( GetTextLocalPosition(renderedLayers, id, it.position, &inverseTransformedPoint) )
					stateDidChange = stb_textedit_drag(&state->text, &state->textEditState, inverseTransformedPoint.x, inverseTransformedPoint.y);
			}
		}

		if(state->isHovering != wasHovering || state->isFocused != wasFocused)
		{
			if(!state->isFocused && wasFocused)
				OnLostFocus();

			stateDidChange = true;
		}
	}

	return stateDidChange;
}

/**
 * The text field lost focus, either through tab, click, or submit.
 */
void VuoTextFieldInternal::OnLostFocus()
{
	if(validateTextInput != NULL)
	{
		VuoText modifiedText = NULL;

		if(validateTextInput(state->text, &modifiedText))
		{
			SetText(modifiedText);

			if(modifiedText != NULL)
			{
				VuoRetain(modifiedText);
				VuoRelease(modifiedText);
			}
		}
	}
}

/**
 * Build a new layer group from the current state of this text field.
 */
VuoLayer VuoTextFieldInternal::CreateTextLayer()
{
	if(state->textImageData)
	{
		VuoRelease(state->textImageData);
		state->textImageData = NULL;
		state->textEditState.textImageData = NULL;
	}

	VuoUiThemeTextField* textTheme = static_cast<VuoUiThemeTextField*>(VuoUiTheme_getSpecificTheme(theme, "VuoUiThemeTextField"));
	VuoLocal(textTheme);

	VuoLayer layer = textTheme->render(screenSize,
												screenBackingScaleFactor,
												cursorColor,
												state->text,
												state->placeholder,
												lineCount,
												state->textEditState.cursor,
												state->textEditState.select_start,
												state->textEditState.select_end,
												position,
												width,
												anchor,
												state->isHovering,
												state->isFocused,
												&state->textImageData);

	if(state->textImageData)
	{
		VuoRetain(state->textImageData);
		state->textEditState.textImageData = state->textImageData;
	}

	VuoLayer_setId(layer, id);

	return layer;
}
