/**
 * @file
 * VuoTextField implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTextField.h"
#include "VuoTextFieldInternal.h"

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoTextField",
					"dependencies" : [ "VuoTextFieldInternal" ]
				});
#endif
}

/**
 * Create a new text field instance.
 */
VuoTextField VuoTextField_make(VuoInteger numLines)
{
	VuoTextFieldInternal* textField = new VuoTextFieldInternal(numLines);
	VuoRegister(textField, VuoTextField_free);
	return static_cast<VuoTextField>(textField);
}

/**
 * Delete a VuoTextField
 */
void VuoTextField_free(VuoTextField textFieldPtr)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	delete textField;
}

/**
 * Send a typed character event to a VuoTextField
 */
void VuoTextField_onTypedCharacter(VuoTextField textFieldPtr, VuoText character, VuoModifierKey modifiers)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->OnTypedCharacterEvent(character, modifiers);
}

/**
 * Send a rendered layers event to a VuoTextField.  Returns true if the internal state has changed and needs to be re-rendered.
 * See also VuoTextField_createTextLayer
 */
bool VuoTextField_onRenderedLayers(VuoTextField textFieldPtr, const VuoRenderedLayers* renderedLayers)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	return textField->OnRenderedLayersEvent(renderedLayers);
}

/**
 * Render out the text field in the current state.
 */
VuoLayer VuoTextField_createTextLayer(VuoTextField textFieldPtr)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	return textField->CreateTextLayer();
}

/**
 * Set the number of lines to display when rendering this field.
 */
void VuoTextField_setLineCount(VuoTextField textFieldPtr, VuoInteger lines)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetLineCount(lines);
}

/**
 * Set the position for this layer.
 */
void VuoTextField_setLayerPosition(VuoTextField textFieldPtr, VuoPoint2d position)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetPosition(position);
}

/**
 * Set the width of this layer.
 */
void VuoTextField_setLayerWidth(VuoTextField textFieldPtr, VuoReal width)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetWidth(width);
}

/**
 * Set the cursor color.
 */
void VuoTextField_setCursorColor(VuoTextField textFieldPtr, VuoColor color)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetCursorColor(color);
}

/**
 * Set the anchor for this layer.
 */
void VuoTextField_setLayerAnchor(VuoTextField textFieldPtr, VuoAnchor anchor)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetAnchor(anchor);
}

/**
 * Set the theme for this layer.
 */
void VuoTextField_setTheme(VuoTextField textFieldPtr, VuoUiTheme theme)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetTheme(theme);
}

/**
 * Set the text for this field.
 */
void VuoTextField_setText(VuoTextField textFieldPtr, VuoText text)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetText(text);
}

/**
 * Set the placeholder text for this field.
 */
void VuoTextField_setPlaceholderText(VuoTextField textFieldPtr, VuoText placeholder)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetPlaceholderText(placeholder);
}

/**
 * Sets an optional function callback to be invoked when OnTypedCharacterEvent is fired.
 * Callback should returns true if input is valid, false otherwise.
 */
void VuoTextField_setValidateCharInputCallback(VuoTextField textFieldPtr, bool (*validateCharInputCallback)(const VuoText current, uint32_t append))
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetValidateCharInputCallback(validateCharInputCallback);
}

/**
 * Optional function callback to be invoked when text field loses focus.  Use this to validate user input and (if necessary) change it.
 * Returns true if the text has been modified (in which case modifiedText should contain the new text).
 */
void VuoTextField_setValidateTextInputCallback(VuoTextField textFieldPtr, bool (*validateTextInputCallback)(const VuoText current, VuoText* modifiedText))
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	textField->SetValidateTextInputCallback(validateTextInputCallback);
}

/**
 * Return a VuoText string of the currently displayed value.
 */
VuoText VuoTextField_getText(VuoTextField textFieldPtr)
{
	VuoTextFieldInternal* textField = static_cast<VuoTextFieldInternal*>(textFieldPtr);
	return textField->GetText();
}
