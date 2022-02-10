/**
 * @file
 * vuo.ui.textarea node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
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

VuoModuleMetadata({
	"title": "Make Text Area",
	"keywords": [
		"string",
		"gui", "user interface", "interact", "widget", "control", "form",
		"enter", "input", "write", "submit", "type", "edit",
	],
	"version": "1.0.0",
	"dependencies": [ "VuoKeyboard", "VuoClipboard", "VuoTextField" ],
	"node": {
		"isDeprecated": true,  // Not ready for release yet. https://b33p.net/kosada/vuo/vuo/-/issues/11630#note_2116791
		"exampleCompositions" : [ ],
	},
});

typedef struct
{
	VuoRenderedLayers renderedLayers;
	VuoTextField textField;
	VuoKeyboard* keyboardListener;
	bool isFirstEvent;
	void (*updatedLayer)(VuoLayer layer);
	void (*updatedValue)(VuoText text);

	struct {
		VuoText value;
		VuoText placeholderText;
		VuoInteger lines;
		VuoAnchor anchor;
		VuoPoint2d position;
		VuoReal width;
		VuoUiTheme theme;
	} prior;
} NodeInstanceData;

static void vuo_ui_make_textarea_sessionEnded(void *contextP, VuoText text)
{
	NodeInstanceData *context = contextP;

	if (!VuoText_areEqual(context->prior.value, text))
	{
		if (context->updatedValue)
			context->updatedValue(text);

		VuoRetain(text);
		VuoRelease(context->prior.value);
		context->prior.value = text;
	}
}

NodeInstanceData* nodeInstanceInit()
{
	NodeInstanceData* instance = (NodeInstanceData*)calloc(1, sizeof(NodeInstanceData));
	VuoRegister(instance, free);

	instance->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(instance->renderedLayers);

	instance->isFirstEvent = true;

	instance->textField = VuoTextField_make(2, instance);
	VuoRetain(instance->textField);

	VuoTextField_setSessionEndedCallback(instance->textField, vuo_ui_make_textarea_sessionEnded);

	instance->keyboardListener = VuoKeyboard_make();
	VuoRetain(instance->keyboardListener);

	return instance;
}

void nodeInstanceTriggerStart(  VuoInstanceData(NodeInstanceData*) instance,
								VuoInputData(VuoRenderedLayers) renderedLayers,
								VuoInputData(VuoInteger) lines,
								VuoOutputTrigger(updatedLayer, VuoLayer),
								VuoOutputTrigger(updatedValue, VuoText))
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*instance)->renderedLayers, renderedLayers, &renderingDimensionsChanged);

	(*instance)->updatedLayer = updatedLayer;
	(*instance)->updatedValue = updatedValue;
	(*instance)->prior.lines = lines;
	VuoTextField_setLineCount((*instance)->textField, lines);
	VuoWindowReference window = NULL;
	(void)VuoRenderedLayers_getWindow((*instance)->renderedLayers, &window);

	VuoKeyboard_startListeningForTypingWithCallback((*instance)->keyboardListener,
										NULL,
										NULL,
										^(VuoText c, VuoModifierKey m)
										{
											VuoTextField_onTypedCharacter((*instance)->textField, c, m);
											(*instance)->updatedLayer(VuoTextField_createTextLayer((*instance)->textField));
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
	VuoInputData(VuoText) setValue,
	VuoInputEvent({"eventBlocking":"none", "data":"setValue"}) setValueEvent,
	VuoInputData(VuoText, {"default":"Click to enter text"}) placeholderText,
	VuoInputData(VuoInteger, {"default":4, "suggestedMin":2, "suggestedMax":20}) lines,
	VuoInputData(VuoAnchor, {"default":{"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) position,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.1, "suggestedMax":2.0}) width,
	VuoInputData(VuoUiTheme) theme,
	VuoOutputTrigger(updatedLayer, VuoLayer),
	VuoOutputTrigger(updatedValue, VuoText)
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
		VuoTextField_setPlaceholderText((*instance)->textField, placeholderText);
	}
	if (!VuoInteger_areEqual((*instance)->prior.lines, lines))
	{
		doRebuildLayer = true;
		(*instance)->prior.lines = lines;
		VuoTextField_setLineCount((*instance)->textField, lines);
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
		VuoText oldText = VuoTextField_getText((*instance)->textField);
		VuoLocal(oldText);
		if (setValue && !VuoText_areEqual(oldText, setValue))
		{
			doRebuildLayer = true;
			VuoTextField_setText((*instance)->textField, setValue);
			VuoText newValue = VuoTextField_getText((*instance)->textField);
			updatedValue(newValue);

			VuoRetain(newValue);
			VuoRelease((*instance)->prior.value);
			(*instance)->prior.value = newValue;
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
