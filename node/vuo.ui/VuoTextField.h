/**
 * @file
 * VuoTextField interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "node.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "VuoUiTheme.h"
#include "VuoRenderedLayers.h"

typedef void* VuoTextField;	///< Opaque pointer to VuoTextField type.

VuoTextField VuoTextField_make(VuoInteger numLines);

void VuoTextField_free(VuoTextField textFieldPtr);

void VuoTextField_onTypedCharacter(VuoTextField textFieldPtr, VuoText character, VuoModifierKey modifiers);

bool VuoTextField_onRenderedLayers(VuoTextField textFieldPtr, const VuoRenderedLayers* renderedLayers);

VuoLayer VuoTextField_createTextLayer(VuoTextField textFieldPtr);

void VuoTextField_setLineCount(VuoTextField textFieldPtr, VuoInteger lines);

void VuoTextField_setLayerPosition(VuoTextField textFieldPtr, VuoPoint2d position);

void VuoTextField_setLayerWidth(VuoTextField textFieldPtr, VuoReal width);

void VuoTextField_setCursorColor(VuoTextField textFieldPtr, VuoColor color);

void VuoTextField_setLayerAnchor(VuoTextField textFieldPtr, VuoAnchor anchor);

void VuoTextField_setTheme(VuoTextField textFieldPtr, VuoUiTheme theme);

void VuoTextField_setText(VuoTextField textFieldPtr, VuoText text);

void VuoTextField_setPlaceholderText(VuoTextField textFieldPtr, VuoText placeholder);

void VuoTextField_setValidateCharInputCallback(VuoTextField textFieldPtr, bool (*validateCharInputCallback)(const VuoText current, uint32_t append));

void VuoTextField_setValidateTextInputCallback(VuoTextField textFieldPtr, bool (*validateTextInputCallback)(const VuoText current, VuoText* modifiedText));

VuoText VuoTextField_getText(VuoTextField textFieldPtr);

#ifdef __cplusplus
}
#endif
