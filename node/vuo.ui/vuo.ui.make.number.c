/**
 * @file
 * vuo.ui.number node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoUiTheme.h"
#include "VuoList_VuoUiTheme.h"
#include "VuoRenderedLayers.h"
#include "../vuo.keyboard/VuoKeyboard.h"
#include "VuoEventLoop.h"
#include <dispatch/dispatch.h>
#include "VuoTextField.h"
#include "../vuo.text/VuoNumberFormat.h"

VuoModuleMetadata({
					  "title" : "Make Number Field",
					  "keywords" : [ "theme", "interface", "ui", "interact", "input", "write", "submit", "digit" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoNumberFormat",
						  "VuoTextField",
					  ],
					  "node" : {
 						  "isDeprecated": true, // Not ready for release yet. https://b33p.net/kosada/node/13475
						  "exampleCompositions" : [ ]
					  }
				 });

typedef struct
{
	VuoTextField textField;
	VuoKeyboard* keyboardListener;
	void (*updatedNumber)(VuoReal value);
	void (*updatedLayer)(VuoLayer layer);
} NodeInstanceData;


static bool validateCharInput(const VuoText current, uint32_t unicode, bool allowDecimals)
{
	// Valid characters for numbers
	static const char numeralChars[] = "-.,0123456789";

	VuoText inputAsText = VuoText_makeFromUtf32(&unicode, 1);
	VuoLocal(inputAsText);

	if( !VuoText_findFirstOccurrence(numeralChars, inputAsText, 1) )
		return false;

	// Checking against both ',' and '.' is not great since it means
	// thousandth separators get rejected when in integer mode.
	// @todo
	if( !allowDecimals && (unicode == '.' || unicode == ',') )
		return false;

	return true;
}

static bool validateCharInput_Decimal(const VuoText current, uint32_t unicode)
{
	return validateCharInput(current, unicode, true);
}

static bool validateCharInput_Integer(const VuoText current, uint32_t unicode)
{
	return validateCharInput(current, unicode, false);
}

static bool validateTextInput(const VuoText current, VuoText* modifiedText)
{
	// strtod parses str according to the current locale
	double result = strtod(current, NULL);
	*modifiedText = VuoNumberFormat_format(result, VuoNumberFormat_Decimal, 1, 0, 16, false);

	return true;
}

NodeInstanceData* nodeInstanceInit()
{
	NodeInstanceData* instance = (NodeInstanceData*)calloc(1, sizeof(NodeInstanceData));
	VuoRegister(instance, free);

	instance->textField = VuoTextField_make(true);
	VuoRetain(instance->textField);

	instance->keyboardListener = VuoKeyboard_make();
	VuoRetain(instance->keyboardListener);

	return instance;
}

static void updatedText(NodeInstanceData* instance)
{
	VuoText str = VuoTextField_getText(instance->textField);
	VuoLocal(str);
	instance->updatedNumber(strtod(str, NULL));
}

void nodeInstanceTriggerStart(  VuoInstanceData(NodeInstanceData*) instance,
								VuoInputData(VuoRenderedLayers) renderedLayers,
								VuoInputData(VuoInteger) maximumDecimalPoints,
								VuoOutputTrigger(updatedNumber, VuoReal),
								VuoOutputTrigger(updatedLayer, VuoLayer))
{
	(*instance)->updatedLayer = updatedLayer;
	(*instance)->updatedNumber = updatedNumber;
	VuoTextField textField = (*instance)->textField;
	VuoWindowReference window = NULL;
	(void)VuoRenderedLayers_getWindow(renderedLayers, &window);

	VuoTextField_setValidateCharInputCallback(textField, maximumDecimalPoints < 1 ? &validateCharInput_Integer : &validateCharInput_Decimal);
	VuoTextField_setValidateTextInputCallback(textField, &validateTextInput);

	VuoKeyboard_startListeningForTypingWithCallback((*instance)->keyboardListener,
										NULL,
										NULL,
										^(VuoText c, VuoModifierKey m)
										{
											VuoTextField_onTypedCharacter(textField, c, m);
											(*instance)->updatedLayer(VuoTextField_createTextLayer(textField));
											updatedText(*instance);
										},
										window);
}

void nodeInstanceTriggerStop(   VuoInstanceData(NodeInstanceData*) instance,
								VuoOutputTrigger(updatedLayer, VuoLayer))
{
	VuoKeyboard_stopListening((*instance)->keyboardListener);
}

void nodeInstanceEvent
(
	VuoInstanceData(NodeInstanceData*) instance,
	VuoInputData(VuoRenderedLayers) renderedLayers,
	VuoInputEvent({"eventBlocking":"wall", "data":"renderedLayers"}) renderedLayersEvent,
	VuoInputData(VuoReal, {"default":0}) value,
	VuoInputEvent({"data":"value"}) valueEvent,
	VuoInputData(VuoReal, {"default":0}) placeholderValue,
	VuoInputData(VuoInteger, {"default":2, "suggestedMin":0, "suggestedMax":16, "suggestedStep":1}) maximumDecimalPoints,
	VuoInputData(VuoColor, {"default":{"r":1, "g":1, "b":1, "a":1}}) cursorColor,
	VuoInputData(VuoAnchor, {"default":{"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
	VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}}) position,
	VuoInputData(VuoUiTheme) theme,
	VuoOutputTrigger(updatedNumber, VuoReal),
	VuoOutputTrigger(updatedLayer, VuoLayer)
)
{
	bool doRebuildLayer = false;

	if(!renderedLayersEvent)
	{
		doRebuildLayer = true;

		VuoText value_string = VuoNumberFormat_format(value, VuoNumberFormat_Decimal, 1, 0, maximumDecimalPoints, false);
		VuoText placeholder_string = VuoNumberFormat_format(placeholderValue, VuoNumberFormat_Decimal, 1, 0, maximumDecimalPoints, false);
		VuoLocal(value_string);
		VuoLocal(placeholder_string);
		VuoTextField_setText((*instance)->textField, value_string);
		VuoTextField_setPlaceholderText((*instance)->textField, placeholder_string);
		VuoTextField_setValidateCharInputCallback((*instance)->textField, maximumDecimalPoints < 1 ? &validateCharInput_Integer : &validateCharInput_Decimal);
		VuoTextField_setCursorColor((*instance)->textField, cursorColor);
		VuoTextField_setLayerPosition((*instance)->textField, position);
		VuoTextField_setLayerAnchor((*instance)->textField, anchor);
		VuoTextField_setTheme((*instance)->textField, theme);
	}
	else
	{
		doRebuildLayer = VuoTextField_onRenderedLayers((*instance)->textField, &renderedLayers);
	}

	if(doRebuildLayer)
		updatedLayer( VuoTextField_createTextLayer((*instance)->textField) );

	if(valueEvent)
		updatedText(*instance);
}

void nodeInstanceFini( VuoInstanceData(NodeInstanceData*) instance )
{
	VuoRelease((*instance)->textField);
	VuoRelease((*instance)->keyboardListener);
}

