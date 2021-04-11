/**
 * @file
 * vuo.ui.text node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
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
					  "title" : "Make Text Field",
					  "keywords" : [
						  "gui", "user interface", "interact", "widget", "control", "form",
						  "enter", "input", "write", "submit", "type", "edit"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [ "VuoTextField" ],
					  "node" : {
						  "exampleCompositions": [ "EnterURLAndShowImage.vuo" ],
					  }
				 });

typedef struct
{
	VuoRenderedLayers renderedLayers;
	VuoTextField textField;
	VuoKeyboard* keyboardListener;
	void (*updatedLayer)(VuoLayer layer);
	void (*updatedValue)(VuoText text);
	bool isFirstEvent;

	struct {
		VuoText value;
		VuoText placeholderText;
		VuoAnchor anchor;
		VuoPoint2d position;
		VuoReal width;
		VuoUiTheme theme;
	} prior;
} NodeInstanceData;

static void vuo_ui_make_text_sessionEnded(void *contextP, VuoText text)
{
	NodeInstanceData *context = contextP;

	if (VuoText_isEmpty(text))
		// Treat emptystring and NULL as equal.
		text = NULL;

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

	instance->textField = VuoTextField_make(1, instance);
	VuoRetain(instance->textField);

	VuoTextField_setSessionEndedCallback(instance->textField, vuo_ui_make_text_sessionEnded);

	instance->keyboardListener = VuoKeyboard_make();
	VuoRetain(instance->keyboardListener);

	return instance;
}

void nodeInstanceTriggerStart(  VuoInstanceData(NodeInstanceData*) instance,
								VuoInputData(VuoRenderedLayers) renderedLayers,
								VuoOutputTrigger(updatedLayer, VuoLayer),
								VuoOutputTrigger(updatedValue, VuoText))
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*instance)->renderedLayers, renderedLayers, &renderingDimensionsChanged);

	VuoTextField tf = (*instance)->textField;
	(*instance)->updatedLayer = updatedLayer;
	(*instance)->updatedValue = updatedValue;
	VuoWindowReference window = NULL;
	(void)VuoRenderedLayers_getWindow((*instance)->renderedLayers, &window);

	VuoKeyboard_startListeningForTypingWithCallback((*instance)->keyboardListener,
										NULL,
										NULL,
										^(VuoText c, VuoModifierKey m)
										{
											VuoTextField_onTypedCharacter(tf, c, m);
											(*instance)->updatedLayer(VuoTextField_createTextLayer(tf));
										},
										window);
}

void nodeInstanceTriggerStop(  VuoInstanceData(NodeInstanceData*) instance,
								VuoOutputTrigger(updatedLayer, VuoLayer))
{
	VuoKeyboard_stopListening((*instance)->keyboardListener);
}

void nodeInstanceEvent
(
	VuoInstanceData(NodeInstanceData*) instance,
	VuoInputData(VuoRenderedLayers, {"name":"Window"}) renderedLayers,
	VuoInputEvent({"data":"renderedLayers"}) renderedLayersEvent,
	// VuoInputData(VuoText) label,
	VuoInputData(VuoText) setValue,
	VuoInputEvent({"eventBlocking":"none", "data":"setValue"}) setValueEvent,
	VuoInputData(VuoText, {"default":"Click to enter text"}) placeholderText,
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
		VuoText _place = VuoText_replace(placeholderText, "\n", "⏎");
		VuoLocal(_place);
		VuoTextField_setPlaceholderText((*instance)->textField, _place);
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
		VuoText _setValueLabel = VuoText_replace(setValue, "\n", "⏎");
		VuoLocal(_setValueLabel);
		VuoText oldText = VuoTextField_getText((*instance)->textField);
		VuoLocal(oldText);

		if (VuoText_isEmpty(oldText))
			// Treat emptystring and NULL as equal.
			oldText = NULL;

		if (!VuoText_areEqual(oldText, _setValueLabel))
		{
			doRebuildLayer = true;
			VuoTextField_setText((*instance)->textField, _setValueLabel);
			VuoText newValue = VuoTextField_getText((*instance)->textField);
			updatedValue(newValue);

			if (VuoText_isEmpty(newValue))
				// Treat emptystring and NULL as equal.
				newValue = NULL;

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
