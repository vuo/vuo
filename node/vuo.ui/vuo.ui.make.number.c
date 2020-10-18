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
#include <CoreFoundation/CoreFoundation.h>
#include <xlocale.h>

VuoModuleMetadata({
					  "title" : "Make Number Field",
					  "keywords" : [
						  "gui", "user interface", "interact", "widget", "control", "form",
						  "enter", "input", "write", "submit", "type", "edit", "digit",
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoNumberFormat",
						  "VuoTextField",
					  ],
					  "node" : {
						  "exampleCompositions": [ "ConvertFahrenheitToCelsius.vuo" ],
					  }
				 });

typedef struct
{
	bool isFirstEvent;
	VuoRenderedLayers renderedLayers;
	VuoTextField textField;
	VuoKeyboard* keyboardListener;
	void (*updatedLayer)(VuoLayer layer);
	void (*updatedValue)(VuoReal value);

	struct {
		VuoReal value;
		VuoText placeholderText;
		VuoInteger maximumDecimalPlaces;
		VuoAnchor anchor;
		VuoPoint2d position;
		VuoReal width;
		VuoUiTheme theme;
	} prior;
} NodeInstanceData;


static bool validateCharInput(const VuoText current, uint32_t newChar, uint16_t position, bool allowDecimals)
{
	// Valid characters for numbers
	static const char numeralChars[] = "-.,0123456789";

	VuoText inputAsText = VuoText_makeFromUtf32(&newChar, 1);
	VuoLocal(inputAsText);

	if( !VuoText_findFirstOccurrence(numeralChars, inputAsText, 1) )
		return false;

	// Only allow a single minus sign, at the beginning.
	size_t currentLen = current ? strlen(current) : 0;
	if (newChar == '-' && position > 0)
		return false;

	// Only allow a single decimal.
	if (newChar == '.' || newChar == ',')
		for (int i = 0; i < currentLen; ++i)
			if (current[i] == '.' || current[i] == ',')
				return false;

	// Checking against both ',' and '.' is not great since it means
	// thousandth separators get rejected when in integer mode.
	// @todo
	if( !allowDecimals && (newChar == '.' || newChar == ',') )
		return false;

	return true;
}

static bool validateCharInput_Decimal(const VuoText current, uint32_t newChar, uint16_t position)
{
	return validateCharInput(current, newChar, position, true);
}

static bool validateCharInput_Integer(const VuoText current, uint32_t newChar, uint16_t position)
{
	return validateCharInput(current, newChar, position, false);
}

static void vuo_ui_make_number_parseNumber(VuoText text, VuoReal *outputNumber)
{
	if (VuoText_isEmpty(text))
	{
		*outputNumber = INFINITY;
		return;
	}

	// The C locale doesn't necessarily match the macOS locale; ensure that it does for this conversion.
	CFLocaleRef localeCF = CFLocaleCopyCurrent();
	VuoText localeIdentifier = VuoText_makeFromCFString(CFLocaleGetIdentifier(localeCF));
	VuoLocal(localeIdentifier);
	CFRelease(localeCF);

	locale_t locale = newlocale(LC_ALL_MASK, localeIdentifier, NULL);
	locale_t oldLocale = uselocale(locale);
	if (oldLocale != LC_GLOBAL_LOCALE)
		freelocale(oldLocale);

	// strtod parses str according to the current locale
	char *endptr;
	*outputNumber = strtod(text, &endptr);

	if (text == endptr)
		*outputNumber = INFINITY;
}

static bool validateTextInput(void *instanceP, const VuoText current, VuoText* modifiedText)
{
	NodeInstanceData *instance = instanceP;

	if (VuoText_isEmpty(current))
	{
		*modifiedText = NULL;
		return true;
	}

	VuoReal number;
	vuo_ui_make_number_parseNumber(current, &number);
	if (!isinf(number))
		*modifiedText = VuoNumberFormat_format(number, VuoNumberFormat_Decimal, 1, 0, instance->prior.maximumDecimalPlaces, false);
	else
		*modifiedText = NULL;

	return true;
}

static void vuo_ui_make_number_sessionEnded(void *contextP, VuoText text)
{
	NodeInstanceData *context = contextP;

	VuoReal number;
	vuo_ui_make_number_parseNumber(text, &number);
	if (context->updatedValue && !isinf(number) && !VuoReal_areEqual(context->prior.value, number))
		context->updatedValue(number);
	context->prior.value = number;
}

NodeInstanceData* nodeInstanceInit()
{
	NodeInstanceData* instance = (NodeInstanceData*)calloc(1, sizeof(NodeInstanceData));
	VuoRegister(instance, free);

	instance->isFirstEvent = true;

	instance->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(instance->renderedLayers);

	instance->textField = VuoTextField_make(1, instance);
	VuoRetain(instance->textField);

	VuoTextField_setSessionEndedCallback(instance->textField, vuo_ui_make_number_sessionEnded);

	instance->keyboardListener = VuoKeyboard_make();
	VuoRetain(instance->keyboardListener);

	instance->prior.value = INFINITY;

	return instance;
}

void nodeInstanceTriggerStart(  VuoInstanceData(NodeInstanceData*) instance,
								VuoInputData(VuoRenderedLayers) renderedLayers,
								VuoInputData(VuoInteger) maximumDecimalPlaces,
								VuoOutputTrigger(updatedLayer, VuoLayer),
								VuoOutputTrigger(updatedValue, VuoReal))
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*instance)->renderedLayers, renderedLayers, &renderingDimensionsChanged);

	(*instance)->updatedLayer = updatedLayer;
	(*instance)->updatedValue = updatedValue;
	(*instance)->prior.maximumDecimalPlaces = maximumDecimalPlaces;
	VuoTextField textField = (*instance)->textField;
	VuoWindowReference window = NULL;
	(void)VuoRenderedLayers_getWindow((*instance)->renderedLayers, &window);

	VuoTextField_setValidateCharInputCallback(textField, maximumDecimalPlaces < 1 ? &validateCharInput_Integer : &validateCharInput_Decimal);
	VuoTextField_setValidateTextInputCallback(textField, &validateTextInput);

	VuoKeyboard_startListeningForTypingWithCallback((*instance)->keyboardListener,
										NULL,
										NULL,
										^(VuoText c, VuoModifierKey m)
										{
											VuoTextField_onTypedCharacter(textField, c, m);
											(*instance)->updatedLayer(VuoTextField_createTextLayer(textField));
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
	VuoInputData(VuoRenderedLayers, {"name":"Window"}) renderedLayers,
	VuoInputEvent({"data":"renderedLayers"}) renderedLayersEvent,
	VuoInputData(VuoReal, {"default":0, "auto":infinity, "autoSupersedesDefault":true}) setValue,
	VuoInputEvent({"eventBlocking":"none", "data":"setValue"}) setValueEvent,
	VuoInputData(VuoText, {"default":"Click to enter a number"}) placeholderText,
	VuoInputData(VuoInteger, {"default":2, "suggestedMin":0, "suggestedMax":16, "suggestedStep":1}) maximumDecimalPlaces,
	VuoInputData(VuoAnchor, {"default":{"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) position,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.1, "suggestedMax":2.0}) width,
	VuoInputData(VuoUiTheme) theme,
	VuoOutputTrigger(updatedLayer, VuoLayer),
	VuoOutputTrigger(updatedValue, VuoReal)
)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*instance)->renderedLayers, renderedLayers, &renderingDimensionsChanged);

	bool doRebuildLayer = false;
	if (!VuoText_areEqual((*instance)->prior.placeholderText, placeholderText))
	{
		doRebuildLayer = true;
		VuoRetain(placeholderText);
		VuoRelease((*instance)->prior.placeholderText);
		(*instance)->prior.placeholderText = placeholderText;
		VuoText _place = VuoText_replace(placeholderText, "\n", "⏎");
		VuoLocal(_place);
		VuoTextField_setPlaceholderText((*instance)->textField, _place);
	}
	if (!VuoInteger_areEqual((*instance)->prior.maximumDecimalPlaces, maximumDecimalPlaces))
	{
		doRebuildLayer = true;
		(*instance)->prior.maximumDecimalPlaces = maximumDecimalPlaces;
		VuoTextField_setValidateCharInputCallback((*instance)->textField, maximumDecimalPlaces < 1 ? &validateCharInput_Integer : &validateCharInput_Decimal);
	}
	if (!VuoAnchor_areEqual((*instance)->prior.anchor, anchor))
	{
		doRebuildLayer = true;
		(*instance)->prior.anchor = anchor;
		VuoTextField_setLayerAnchor((*instance)->textField, anchor);
	}
	if (!VuoPoint2d_areEqual((*instance)->prior.position, position))
	{
		doRebuildLayer = true;
		(*instance)->prior.position = position;
		VuoTextField_setLayerPosition((*instance)->textField, position);
	}
	if (!VuoReal_areEqual((*instance)->prior.width, width))
	{
		doRebuildLayer = true;
		(*instance)->prior.width = width;
		VuoTextField_setLayerWidth((*instance)->textField, width);
	}
	if (!VuoUiTheme_areEqual((*instance)->prior.theme, theme))
	{
		doRebuildLayer = true;
		VuoRetain(theme);
		VuoRelease((*instance)->prior.theme);
		(*instance)->prior.theme = theme;
		VuoTextField_setTheme((*instance)->textField, theme);
	}

	if (setValueEvent || (*instance)->isFirstEvent)
	{
		(*instance)->isFirstEvent = false;

		if (!isinf(setValue))
		{
			VuoText value_string = VuoNumberFormat_format(setValue, VuoNumberFormat_Decimal, 1, 0, maximumDecimalPlaces, false);
			VuoLocal(value_string);
			VuoTextField_setText((*instance)->textField, value_string);
		}
		else
			VuoTextField_setText((*instance)->textField, NULL);

		VuoText text = VuoTextField_getText((*instance)->textField);
		VuoLocal(text);
		VuoReal number;
		vuo_ui_make_number_parseNumber(text, &number);
		if (!VuoReal_areEqual((*instance)->prior.value, number))
		{
			doRebuildLayer = true;
			if (!isinf(number))
				updatedValue(number);
			(*instance)->prior.value = number;
		}
	}

	if (renderedLayersEvent)
		doRebuildLayer |= VuoTextField_onRenderedLayers((*instance)->textField, &(*instance)->renderedLayers);

	if (doRebuildLayer)
		updatedLayer(VuoTextField_createTextLayer((*instance)->textField));
}

void nodeInstanceFini( VuoInstanceData(NodeInstanceData*) instance )
{
	VuoRenderedLayers_release((*instance)->renderedLayers);
	VuoRelease((*instance)->textField);
	VuoRelease((*instance)->keyboardListener);
	VuoRelease((*instance)->prior.placeholderText);
	VuoRelease((*instance)->prior.theme);
}
